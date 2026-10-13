/**
 * @file bsp_pwm.c
 * @brief PWM 板级驱动实现（LEDC 硬件 PWM，输出到 LED）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 封装 ESP-IDF 的 LEDC（LED Controller）驱动，对外提供三个函数：
 *   bsp_pwm_init()     —— 配置 LEDC 定时器与通道，把 PWM 输出绑到 LED 引脚
 *   bsp_pwm_set_duty() —— 软件方式直接设置占空比
 *   bsp_pwm_fade_to()  —— 硬件渐变，在指定时间内平滑过渡到目标占空比
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * 【LEDC 硬件 PWM 的分辨率 / 频率 / 占空比关系】
 *   LEDC 是 ESP32-S3 内部独立于 CPU 的硬件 PWM 模块：定时器按设定频率计数，
 *   计满 2^resolution 个计数周期后回卷，期间比较器按 duty 翻转输出电平，从而在引脚上
 *   产生固定频率、占空比可调的方波，CPU 无需干预。
 *   三者关系：**占空比 = duty / 2^resolution**。
 *   本工程分辨率 BSP_PWM_DUTY_RES = LEDC_TIMER_10_BIT，即 2^10 = 1024 级，
 *   duty 取值范围 0 ~ 1023（BSP_PWM_DUTY_MAX = 1023）：duty=0 全灭、duty=1023 全亮。
 *   频率 BSP_PWM_FREQ_HZ = 1000Hz（1kHz），远高于人眼闪烁感知频率，故高频通断被
 *   视觉暂留积分成亮度变化，duty 越大越亮，即可做呼吸灯。
 *   注意分辨率越高（量化越细）能达到的最高频率越低，二者受 LEDC 内部时钟分频约束。
 * 【软件 PWM vs 硬件 PWM】
 *   - 软 PWM：由 bsp_swpwm_demo() 在任务循环里反复调用 bsp_pwm_set_duty() 逐步改
 *     占空比；输出仍是硬件方波，只是「改变占空比的节奏」由 CPU 控制；
 *   - 硬 PWM：由 bsp_pwm_fade_to() 交给 LEDC 硬件的渐变引擎自动把占空比从当前值
 *     线性过渡到目标值，期间 CPU 几乎不参与，过渡更平滑。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 定时器号/通道号/频率/位宽/最大占空比均来自 board.h 的 BSP_PWM_* 宏，输出引脚来自
 * BSP_LED0_GPIO（IO1），本文件不写死数字。
 * 指南章节：第十七章 SW_PWM、第十八章 HW_PWM。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 *   1) PWM 通道与引脚在 ledc_channel_config() 时绑定，一个通道固定输出到一个 GPIO，
 *      要换引脚必须重新配置通道；
 *   2) ledc_set_duty() 只写影子寄存器，必须再调用 ledc_update_duty() 才真正生效
 *      （本驱动的 bsp_pwm_set_duty() 已把两步合并）；
 *   3) 渐变功能需先 ledc_fade_func_install() 安装，重复安装会返回错误，本工程忽略；
 *   4) 本板 LED 为灌电流接法（见 board.h：低电平点亮），因此「占空比越大越亮」的
 *      实际观感以硬件接法为准，本工程按正逻辑演示亮度变化。
 */

#include "bsp_pwm.h"
#include "board.h"
#include "driver/ledc.h"
#include "esp_log.h"

/* 日志标签：打印 PWM 相关日志时统一显示为 "bsp_pwm" */
static const char *TAG = "bsp_pwm";

/**
 * @brief 初始化 LEDC 定时器与通道（输出到 LED GPIO）
 *
 * 逻辑流程：
 *   ① 配置并初始化 LEDC 定时器（速度模式 / 定时器号 / 分辨率 / 频率 / 时钟源）
 *   ② 配置并初始化 LEDC 通道，把它绑定到 LED 引脚，初始占空比 0（不亮）
 *   ③ 任一步失败打印错误并返回对应错误码
 */
esp_err_t bsp_pwm_init(void)
{
    /* ① 定时器配置：
     * speed_mode      ：速度模式。LEDC_LOW_SPEED_MODE 为低速模式（可在任意时刻
     *                   改占空比，最常用）；高速模式用于特殊场景；
     * timer_num       ：使用哪个 LEDC 定时器（BSP_PWM_LEDC_TIMER = LEDC_TIMER_0）；
     * duty_resolution ：占空比分辨率（BSP_PWM_DUTY_RES = LEDC_TIMER_10_BIT），
     *                   10 位 -> duty 范围 0~1023，占空比 = duty/1024；
     * freq_hz         ：PWM 频率（BSP_PWM_FREQ_HZ = 1000Hz），决定方波周期；
     * clk_cfg         ：时钟源，LEDC_AUTO_CLK 让驱动自动选择合适时钟。 */
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = BSP_PWM_LEDC_TIMER,
        .duty_resolution = BSP_PWM_DUTY_RES,
        .freq_hz = BSP_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t ret = ledc_timer_config(&timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc_timer_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ② 通道配置（把某个 LEDC 通道与 GPIO、定时器绑定）：
     * channel     ：使用哪个 LEDC 通道（BSP_PWM_LEDC_CHANNEL = LEDC_CHANNEL_0）；
     * timer_sel   ：该通道使用哪个定时器（须与上面的 timer_num 一致）；
     * intr_type   ：是否产生中断，LEDC_INTR_DISABLE 表示 PWM 纯硬件输出、不产生中断；
     * gpio_num    ：输出引脚（BSP_LED0_GPIO = IO1，即板上 LED）；
     * duty        ：初始占空比 0（上电先灭，避免突然亮起）；
     * hpoint      ：高电平相位起点，0 表示从周期起点即开始高电平。 */
    ledc_channel_config_t ch = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = BSP_PWM_LEDC_CHANNEL,
        .timer_sel = BSP_PWM_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = BSP_LED0_GPIO,
        .duty = 0,
        .hpoint = 0,
    };
    ret = ledc_channel_config(&ch);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc_channel_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ③ 初始化成功，打印频率与引脚方便对照硬件确认输出位置 */
    ESP_LOGI(TAG, "LEDC PWM init: %d Hz, GPIO %d", BSP_PWM_FREQ_HZ, BSP_LED0_GPIO);
    return ESP_OK;
}

/**
 * @brief 软件方式设置 PWM 占空比（0 ~ BSP_PWM_DUTY_MAX）
 *
 * LEDC 修改占空比分两步：
 *   ① ledc_set_duty()    —— 把新占空比写入影子寄存器（尚未生效）；
 *   ② ledc_update_duty() —— 把影子寄存器同步到硬件，占空比真正改变。
 * 本函数把两步合并，任一步失败即返回错误码。
 *
 * @param duty 占空比，10 位分辨率下 0~1023（BSP_PWM_DUTY_MAX = 1023）
 * @return ESP_OK 成功；否则为对应错误码
 */
esp_err_t bsp_pwm_set_duty(uint32_t duty)
{
    esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, BSP_PWM_LEDC_CHANNEL, duty);
    if (ret != ESP_OK) {
        return ret;   /* 写影子寄存器失败，直接返回 */
    }
    return ledc_update_duty(LEDC_LOW_SPEED_MODE, BSP_PWM_LEDC_CHANNEL);   /* 同步到硬件生效 */
}

/**
 * @brief 硬件渐变到目标占空比（time_ms 毫秒内平滑完成）
 *
 * 逻辑流程：
 *   ① 安装 LEDC 渐变功能（重复安装会返回错误，故作忽略处理）
 *   ② 设置渐变参数：目标占空比 + 渐变时间
 *   ③ 启动渐变并等待完成（LEDC_FADE_WAIT_DONE 阻塞至渐变结束）
 *
 * @param target_duty 目标占空比（0~BSP_PWM_DUTY_MAX）
 * @param time_ms     从当前占空比过渡到目标值所需的时间（毫秒），越大过渡越平缓
 * @return ESP_OK 成功；否则为对应错误码
 */
esp_err_t bsp_pwm_fade_to(uint32_t target_duty, int time_ms)
{
    /* ① 安装渐变功能（重复安装返回错误，忽略）。必须先安装才能使用渐变 API */
    ledc_fade_func_install(0);

    /* ② 设置渐变：把通道当前占空比在 time_ms 内线性过渡到 target_duty */
    esp_err_t ret = ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, BSP_PWM_LEDC_CHANNEL,
                                            target_duty, time_ms);
    if (ret != ESP_OK) {
        return ret;
    }
    /* ③ 启动渐变；LEDC_FADE_WAIT_DONE 表示本函数阻塞等待渐变完成后才返回 */
    return ledc_fade_start(LEDC_LOW_SPEED_MODE, BSP_PWM_LEDC_CHANNEL, LEDC_FADE_WAIT_DONE);
}
