/**
 * @file bsp_led.c
 * @brief LED 板级驱动实现（用户 LED 接 IO1，低电平点亮）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 封装 ESP-IDF 的 GPIO 输出驱动，对外提供三个函数：
 *   bsp_led_init()   —— 把 LED 所在 GPIO 配置为输出，并默认熄灭
 *   bsp_led_set()    —— 点亮 / 熄灭（入参是逻辑状态，内部换算成实际 GPIO 电平）
 *   bsp_led_toggle() —— 翻转当前状态（亮↔灭）
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * 本板用户 LED 采用「灌电流」接法：LED 阳极接 3.3V，阴极经限流电阻接 IO1，
 * 因此 IO1 输出低电平时回路导通、LED 点亮；输出高电平时回路断开、LED 熄灭，
 * 即 BSP_LED0_ACTIVE_LEVEL = 0（低电平有效）。
 * 为屏蔽这种「反相」细节，驱动内部用一个逻辑状态变量 s_led_on 记录「亮/灭」，
 * 由 bsp_led_set() 按有效电平把逻辑状态换算成实际电平输出；调用者只需记住
 * on=true 表示点亮，无需关心硬件到底是高电平点亮还是低电平点亮。
 *
 * 为什么用 s_led_on 而不直接 gpio_get_level() 读回电平？
 *   因为纯输出引脚读回的是引脚当前电平而非「驱动意图」，且翻转时需要知道当前
 *   逻辑状态；用一个软件变量记住状态，toggle 时取反即可，简单可靠。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 引脚号与有效电平均来自 board.h 的 BSP_LED0_* 宏，本文件不写死任何数字。
 * 指南章节：第十章 LED 实验。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * ① 有效电平 BSP_LED0_ACTIVE_LEVEL=0 决定 set() 的换算方向，切不可把逻辑状态
 *    直接当电平写进 gpio_set_level()；
 * ② 必须先调用 bsp_led_init() 配置好 GPIO，否则 set() 无效；
 * ③ 日志标签统一为 "bsp_led"。
 */

#include "bsp_led.h"
#include "board.h"
#include "driver/gpio.h"
#include "esp_log.h"

/* 日志标签：打印 LED 相关日志时统一显示为 "bsp_led" */
static const char *TAG = "bsp_led";
static bool s_led_on = false;   /* 记录 LED 逻辑状态，避免读回纯输出引脚电平 */

/**
 * @brief 初始化 LED：把 LED 引脚配置为输出，并默认熄灭
 *
 * 逻辑流程：
 *   ① 用 BSP_LED0_GPIO 组装 GPIO 配置（输出模式、无上下拉、不使能中断）
 *   ② 调用 gpio_config() 应用配置，失败则打印错误并立即返回
 *   ③ 调用 bsp_led_set(false) 让 LED 上电即处于熄灭状态
 */
esp_err_t bsp_led_init(void)
{
    gpio_config_t io_conf = {
        /* pin_bit_mask：要配置的引脚掩码。1ULL << GPIO 号 把第 n 位置 1，
         * 表示选中该引脚；用 1ULL（无符号 64 位）是因为该字段为 64 位，
         * 避免移位溢出。 */
        .pin_bit_mask = 1ULL << BSP_LED0_GPIO,
        .mode = GPIO_MODE_OUTPUT,           /* 输出模式：LED 只需被驱动亮灭 */
        .pull_up_en = GPIO_PULLUP_DISABLE,  /* 输出引脚无需内部上拉 */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,     /* 输出引脚不使用中断 */
    };

    /* 应用上面的 GPIO 配置；任一参数非法都会返回错误码 */
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(ret));
        return ret;        /* 配置失败则不谎报成功，交由调用者处理 */
    }

    /* 初始熄灭：统一走逻辑接口，避免在初始化处再次关心有效电平 */
    bsp_led_set(false);
    ESP_LOGI(TAG, "LED initialized on GPIO %d", BSP_LED0_GPIO);   /* 打印所用引脚，便于对照硬件 */
    return ESP_OK;
}

/**
 * @brief 点亮 / 熄灭 LED
 *
 * 逻辑流程：
 *   ① 依据 BSP_LED0_ACTIVE_LEVEL 把逻辑状态 on 换算成实际 GPIO 电平
 *   ② 把逻辑状态记录到 s_led_on，供 toggle 判断当前状态
 *   ③ 把换算后的电平输出到 BSP_LED0_GPIO
 *
 * @param on true=点亮，false=熄灭（与硬件高/低电平无关）
 */
esp_err_t bsp_led_set(bool on)
{
    /* 有效电平换算：BSP_LED0_ACTIVE_LEVEL==0 表示低电平点亮，
     * 此时 on=1 输出 0、on=0 输出 1（反相）；
     * 若该宏为 1（高电平点亮），则 on 与电平方向一致。 */
    uint32_t level = (BSP_LED0_ACTIVE_LEVEL == 0) ? (on ? 0 : 1) : (on ? 1 : 0);
    s_led_on = on;                                       /* 记住逻辑状态，供 toggle 使用 */
    return gpio_set_level(BSP_LED0_GPIO, level);         /* 写入引脚实际电平 */
}

/**
 * @brief 翻转 LED：亮→灭、灭→亮
 *
 * 逻辑流程：
 *   ① 把记录的逻辑状态 s_led_on 取反
 *   ② 交给 bsp_led_set() 完成电平换算与输出（复用同一套逻辑）
 */
esp_err_t bsp_led_toggle(void)
{
    return bsp_led_set(!s_led_on);   /* 逻辑状态取反后交由 set() 处理 */
}
