/**
 * @file hwpwm_demo.c
 * @brief HW_PWM 实验示例（对应指南第十八章 HW_PWM）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 初始化 LEDC 硬件 PWM（输出到 LED）；
 * 2) 用 LEDC 硬件的渐变功能，在 1 秒内把占空比从 0 平滑升到 BSP_PWM_DUTY_MAX（渐亮），
 *    再在 1 秒内降回 0（渐暗），如此循环形成呼吸灯；
 * 3) 占空比的变化由硬件渐变引擎自动完成，CPU 几乎不参与（与软 PWM 对比）。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "HW PWM (hardware fade, LED breathing)"（对应宏
 * CONFIG_DEMO_HWPWM），app_main() 就会调用 bsp_hwpwm_demo()。
 */

#include "bsp_pwm.h"
#include "board.h"
#include "esp_log.h"

/* 日志标签：本示例打印的日志会显示为 "hwpwm_demo" */
static const char *TAG = "hwpwm_demo";

void bsp_hwpwm_demo(void)
{
    /* 打印示例说明：硬件渐变实现呼吸灯 */
    ESP_LOGI(TAG, "HW_PWM demo start: LED breathing (hardware fade)");
    /* 初始化 LEDC PWM（定时器 + 通道，输出到 LED 引脚），失败即断言复位 */
    ESP_ERROR_CHECK(bsp_pwm_init());

    while (1) {
        /* 渐亮：在 1000ms 内把占空比硬件渐变到最大值（全亮） */
        ESP_ERROR_CHECK(bsp_pwm_fade_to(BSP_PWM_DUTY_MAX, 1000));
        /* 渐暗：再在 1000ms 内硬件渐变回 0（全灭） */
        ESP_ERROR_CHECK(bsp_pwm_fade_to(0, 1000));
    }
}
