/**
 * @file bsp_key.c
 * @brief 独立按键（BOOT 键接 IO0，低电平有效）板级驱动实现
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 本文件对外提供两类用法、共四个函数：
 *   bsp_key_init()           —— 把按键 GPIO 配置为输入并使能内部上拉（轮询方式用）
 *   bsp_key_scan()           —— 扫描一次按键，软件消抖 10ms，返回是否按下
 *   bsp_key_intr_init()      —— 配置下降沿外部中断并注册 ISR（中断方式用）
 *   bsp_key_intr_triggered() —— 查询中断是否触发过（读取后自动清标志）
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * 【硬件】BOOT 键一端接 IO0、另一端接 GND，按下时把 IO0 拉低，松开时由内部上拉
 *   拉到高电平，所以该引脚为低电平有效（BSP_KEY0_ACTIVE_LEVEL = 0）。
 *
 * 【轮询方式】机械按键在闭合瞬间触点会反复弹跳 5~10ms（抖动），若一检测到低电平
 *   就判定按下，会误判成多次按下。bsp_key_scan() 采用「软件延时消抖」：
 *     ① 外部调用可传 mode=1 支持连按（按住期间每次调用都允许重新判定）；
 *     ② 若当前处于"已松开"状态且读到有效电平，则先延时 10ms 跨过抖动区再复读一次；
 *     ③ 两次都读到有效电平才确认按下并返回 1，同时置 released=0 锁定，
 *        保证一次按下在 mode=0 时只返回一次；
 *     ④ 读到无效电平则恢复 released=1（表示已松开），等待下一次按下。
 *
 * 【外部中断方式】把 IO0 配置为 BSP_KEY0_INTR_TYPE（下降沿触发，正好对应"按下"的
 *   电平跳变），并注册 ISR。ISR 只做最少的动作——把 volatile 标志置位，绝不阻塞、
 *   不打印（ISR 在中断上下文运行，耗时过长会触发看门狗）；真正的业务处理（翻转
 *   LED）由任务轮询 bsp_key_intr_triggered() 完成，即"中断只投递事件、任务再处理"。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 引脚号、有效电平、触发沿均来自 board.h 的 BSP_KEY0_* 宏，本文件不写死任何数字。
 * 指南章节：第十一章 KEY 实验（外部中断部分对应 EXIT 实验）。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * ① 按键低电平有效，必须使能内部上拉，否则引脚悬空时电平乱跳、读数不可信；
 * ② ISR 内严禁调用可能阻塞的函数（vTaskDelay / printf 等）；
 * ③ gpio_install_isr_service() 全系统只能安装一次，重复安装会返回
 *    ESP_ERR_INVALID_STATE，此处将其视为正常（可能其他组件已安装过）；
 * ④ 消抖延时 10ms 会短暂阻塞当前任务，仅适合演示，量产建议改用时间戳方式；
 * ⑤ 日志标签统一为 "bsp_key"。
 */

#include "bsp_key.h"
#include "board.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

/* 日志标签：打印按键相关日志时统一显示为 "bsp_key" */
static const char *TAG = "bsp_key";

/**
 * @brief 初始化按键：把 BOOT 键 GPIO 配置为输入并使能内部上拉（轮询方式）
 *
 * 逻辑流程：
 *   ① 组装 GPIO 配置：输入模式、内部上拉、不使能中断
 *   ② 调用 gpio_config() 应用配置，失败则打印错误并立即返回
 *   ③ 打印所用引脚号，便于对照硬件
 */
esp_err_t bsp_key_init(void)
{
    gpio_config_t io_conf = {
        /* 引脚掩码：1ULL << GPIO 号，把第 n 位置 1 表示选中该引脚（64 位字段） */
        .pin_bit_mask = 1ULL << BSP_KEY0_GPIO,
        .mode = GPIO_MODE_INPUT,              /* 输入模式：读取按键电平 */
        .pull_up_en = GPIO_PULLUP_ENABLE,     /* 按键低电平有效，需要上拉 */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,       /* 轮询方式不使用中断 */
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "KEY initialized on GPIO %d", BSP_KEY0_GPIO);
    return ESP_OK;
}

/**
 * @brief 扫描一次按键（软件消抖 10ms）
 *
 * 逻辑流程：
 *   ① mode=1 时把 released 复位为 1，允许「连按」（按住期间每次调用都能返回）
 *   ② 若处于"已松开"状态且当前读到有效电平：延时 10ms 消抖后复读一次确认
 *   ③ 确认按下则返回 1，并置 released=0 锁定，避免一次按下重复上报
 *   ④ 若读到无效电平，则恢复 released=1，表示按键已松开
 *
 * @param mode 0=不支持连按（一次按下只返回一次）；1=支持连按（按住期间每次调用都返回）
 * @return 0=无按键；1=检测到按下
 */
uint8_t bsp_key_scan(uint8_t mode)
{
    static uint8_t released = 1;   /* 1=已松开（可再次判定），0=已按下（锁定，防止重复上报） */
    uint8_t keyval = 0;            /* 本次扫描结果，默认无按键 */

    if (mode) {
        released = 1;              /* 支持连按：放开锁定，允许本次重新判定 */
    }

    /* 仅在"已松开"且读到有效电平（按下）时进入判定 */
    if (released && (gpio_get_level(BSP_KEY0_GPIO) == BSP_KEY0_ACTIVE_LEVEL)) {
        vTaskDelay(pdMS_TO_TICKS(10));   /* 软件消抖：延时 10ms 跨过触点抖动区 */
        released = 0;                    /* 先锁定，避免抖动期间被重复判定 */
        /* 复读一次，两次都有效才确认真正按下 */
        if (gpio_get_level(BSP_KEY0_GPIO) == BSP_KEY0_ACTIVE_LEVEL) {
            keyval = 1;                  /* 确认按下 */
        }
    } else if (gpio_get_level(BSP_KEY0_GPIO) != BSP_KEY0_ACTIVE_LEVEL) {
        released = 1;                    /* 读到无效电平（已松开），恢复可判定状态 */
    }

    return keyval;                       /* 0=无按键，1=按下 */
}

/* ==================== 外部中断（EXIT） ==================== */

/* 中断触发标志：由 ISR 置位、由任务读取后清除。
 * 必须加 volatile，防止编译器把它缓存在寄存器里，导致任务看不到 ISR 的修改。 */
static volatile bool s_intr_triggered = false;

/**
 * @brief GPIO 中断服务函数（ISR）
 *
 * 逻辑流程：
 *   ① 只把标志置位，通知任务"按键触发过一次"
 *
 * @note IRAM_ATTR 让函数常驻 IRAM，保证在 Flash 操作期间也能执行中断；
 *       中断上下文不可阻塞、不可打印，实际处理交由任务完成。
 */
static void IRAM_ATTR key_isr_handler(void *arg)
{
    /* 中断里只做最少的事：置标志，具体处理交给任务 */
    s_intr_triggered = true;
}

/**
 * @brief 初始化按键外部中断：配置下降沿触发并注册 ISR
 *
 * 逻辑流程：
 *   ① 组装 GPIO 配置：输入、内部上拉、触发沿取自 BSP_KEY0_INTR_TYPE（下降沿）
 *   ② gpio_install_isr_service() 安装全局 GPIO 中断服务（全系统只需一次）
 *   ③ gpio_isr_handler_add() 把 key_isr_handler 绑定到按键引脚
 *   ④ gpio_intr_enable() 使能该引脚中断
 */
esp_err_t bsp_key_intr_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BSP_KEY0_GPIO,
        .mode = GPIO_MODE_INPUT,                /* 输入模式 */
        .pull_up_en = GPIO_PULLUP_ENABLE,       /* 低电平有效，需上拉 */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = BSP_KEY0_INTR_TYPE,        /* 下降沿触发 */
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 安装 GPIO 中断服务（同一时刻只能安装一次）。
     * ESP_INTR_FLAG_EDGE 表示按边沿方式分配中断；若已被别处安装过会返回
     * ESP_ERR_INVALID_STATE，这属于正常情况，不视为错误。 */
    ret = gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "gpio_install_isr_service failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 把中断处理函数绑定到按键引脚；arg 传 NULL（本 ISR 不需要参数） */
    ret = gpio_isr_handler_add(BSP_KEY0_GPIO, key_isr_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_isr_handler_add failed: %s", esp_err_to_name(ret));
        return ret;
    }

    gpio_intr_enable(BSP_KEY0_GPIO);       /* 使能该引脚的中断 */
    ESP_LOGI(TAG, "KEY interrupt enabled on GPIO %d (falling edge)", BSP_KEY0_GPIO);
    return ESP_OK;
}

/**
 * @brief 查询中断是否触发过（读取后自动清除标志）
 *
 * 逻辑流程：
 *   ① 若标志已置位，则清标志并返回 true（只上报一次）
 *   ② 否则返回 false
 *
 * @return true=自上电（或上次读取）以来发生过一次按键中断；false=未发生
 */
bool bsp_key_intr_triggered(void)
{
    if (s_intr_triggered) {
        s_intr_triggered = false;   /* 读取即清除，保证一次中断只被处理一次 */
        return true;
    }
    return false;                   /* 无触发 */
}
