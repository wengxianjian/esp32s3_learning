/**
 * @file esptimer_demo.c
 * @brief ESPTimer 实验示例（对应指南第十四章 ESPTIMER实验）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 初始化 LED，并创建周期 500ms 的软件定时器（ESPTimer）；
 * 2) 定时器每次触发时（在回调里）翻转一次 LED，因此 LED 以 500ms 间隔闪烁；
 * 3) 主循环每 2 秒打印一次定时器累计触发次数（正常应每次约 +4）。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "ESPTimer (software timer, toggle LED)"（对应宏
 * CONFIG_DEMO_ESPTIMER），app_main() 就会调用 bsp_esptimer_demo()。
 */

#include "bsp_timer.h"
#include "board.h"
#include "bsp_led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 日志标签：本示例打印的日志会显示为 "esptimer_demo" */
static const char *TAG = "esptimer_demo";

void bsp_esptimer_demo(void)
{
    /* 打印示例说明：周期由 BSP_ESPTIMER_PERIOD_US 决定（宏单位是 us，
     * 除以 1000 换算成 ms 显示，方便阅读） */
    ESP_LOGI(TAG, "ESPTimer demo start: toggle LED every %d ms", BSP_ESPTIMER_PERIOD_US / 1000);
    /* 初始化 LED。ESP_ERROR_CHECK：初始化失败会打印断言信息并复位，避免带病运行 */
    ESP_ERROR_CHECK(bsp_led_init());
    /* 创建并启动软件定时器；LED 的翻转和计数都在其回调里完成 */
    ESP_ERROR_CHECK(bsp_esptimer_init());

    while (1) {
        /* 主任务本身不做闪烁，只每 2 秒查询一次，演示「定时器独立于任务运行」 */
        vTaskDelay(pdMS_TO_TICKS(2000));
        /* 读取累计触发次数：周期 500ms、打印间隔 2s，正常情况下每次打印约 +4 */
        ESP_LOGI(TAG, "timer fired %lu times", (unsigned long)bsp_esptimer_get_tick_count());
    }
}
