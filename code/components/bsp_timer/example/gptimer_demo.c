/**
 * @file gptimer_demo.c
 * @brief GPTimer 实验示例（对应指南第十五章 GPTIMER实验）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 初始化 LED，并创建周期 500ms 的硬件定时器（GPTimer）；
 * 2) 主循环里轮询定时器的报警次数，一旦计数变化就翻转一次 LED（LED 每 500ms 闪烁）；
 * 3) 每 2 秒打印一次报警次数，与 ESPTimer 示例做对比（观察硬件定时器同样准时）。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "GPTimer (hardware timer, toggle LED)"（对应宏
 * CONFIG_DEMO_GPTIMER），app_main() 就会调用 bsp_gptimer_demo()。
 */

#include "bsp_timer.h"
#include "board.h"
#include "bsp_led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 日志标签：本示例打印的日志会显示为 "gptimer_demo" */
static const char *TAG = "gptimer_demo";

void bsp_gptimer_demo(void)
{
    /* 打印示例说明：周期由 BSP_GPTIMER_PERIOD_US 决定（单位 us，除以 1000 显示为 ms） */
    ESP_LOGI(TAG, "GPTimer demo start: toggle LED every %d ms", BSP_GPTIMER_PERIOD_US / 1000);
    /* 初始化 LED。ESP_ERROR_CHECK：失败会打印断言并复位，避免带病运行 */
    ESP_ERROR_CHECK(bsp_led_init());
    /* 创建并启动硬件定时器；其回调运行在 ISR 上下文，只负责计数 */
    ESP_ERROR_CHECK(bsp_gptimer_init());

    /* 记录上一次看到的报警计数，用于「计数变化才翻转 LED」的边沿判断 */
    uint32_t last_tick = 0;
    /* 记录上一次打印日志的时刻（tick），用于每 2 秒打印一次 */
    TickType_t last_log = xTaskGetTickCount();

    while (1) {
        /* 读取当前报警次数（ISR 里累加） */
        uint32_t tick = bsp_gptimer_get_tick_count();
        /* 计数变化说明定时器又报警了一次：翻转 LED，并更新基准值。
         * 这一步放在任务里做（不是 ISR 里），因为翻转 LED 涉及 GPIO 驱动，更安全 */
        if (tick != last_tick) {
            bsp_led_toggle();
            last_tick = tick;
        }
        /* 距离上次打印已满 2 秒（pdMS_TO_TICKS 把 ms 换算成 tick），打印一次累计次数 */
        if (xTaskGetTickCount() - last_log >= pdMS_TO_TICKS(2000)) {
            last_log = xTaskGetTickCount();
            ESP_LOGI(TAG, "gptimer fired %lu times", (unsigned long)tick);
        }
        /* 每 10ms 轮询一次，兼顾响应速度与 CPU 占用 */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
