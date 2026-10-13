/**
 * @file swpwm_demo.c
 * @brief SW_PWM 实验示例（对应指南第十七章 SW_PWM）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 初始化 LEDC 硬件 PWM（输出到 LED）；
 * 2) 在任务循环里「软件」逐步修改占空比：每 10ms 把 duty 增减 5，
 *    从 0 累加到 BSP_PWM_DUTY_MAX 再反向递减，从而形成 LED 呼吸灯效果；
 * 3) 因为改占空比的节奏由 CPU 逐次控制，所以称为「软件 PWM」。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "SW PWM (software duty change, LED breathing)"（对应宏
 * CONFIG_DEMO_SWPWM），app_main() 就会调用 bsp_swpwm_demo()。
 */

#include "bsp_pwm.h"
#include "board.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 日志标签：本示例打印的日志会显示为 "swpwm_demo" */
static const char *TAG = "swpwm_demo";

void bsp_swpwm_demo(void)
{
    /* 打印示例说明：软件改占空比实现呼吸灯 */
    ESP_LOGI(TAG, "SW_PWM demo start: LED breathing (software duty change)");
    /* 初始化 LEDC PWM（定时器 + 通道，输出到 LED 引脚），失败即断言复位 */
    ESP_ERROR_CHECK(bsp_pwm_init());

    /* 当前占空比，从 0 开始（最暗） */
    uint32_t duty = 0;
    /* 变化方向：+1 表示渐亮（duty 递增），-1 表示渐暗（duty 递减） */
    int dir = 1;

    while (1) {
        /* 按当前方向调整占空比，步长 5/次，数值越大呼吸越快 */
        duty += dir * 5;
        /* 到达上限（BSP_PWM_DUTY_MAX = 1023，全亮）时改为递减，开始渐暗 */
        if (duty >= BSP_PWM_DUTY_MAX) {
            dir = -1;
        /* 回到 0（全灭）时改为递增，开始渐亮 */
        } else if (duty == 0) {
            dir = 1;
        }
        /* 立即把新占空比写入 LEDC 生效（内部含 set_duty + update_duty 两步） */
        bsp_pwm_set_duty(duty);
        /* 每 10ms 调一次占空比，节奏足够细腻，人眼看到平滑的呼吸效果 */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
