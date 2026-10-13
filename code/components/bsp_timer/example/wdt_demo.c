/**
 * @file wdt_demo.c
 * @brief Watchdog 实验示例（对应指南第十六章 WATCH_DOG实验）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 初始化 LED 与任务看门狗（TWDT），并把当前任务加入监视；
 * 2) 每秒喂狗一次、同时翻转 LED，连续喂 5 次，演示「正常喂狗不会复位」；
 * 3) 第 6 秒起停止喂狗，约 BSP_WDT_TIMEOUT_MS（3s）后看门狗超时，板子打印回溯并复位。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "Watchdog (task watchdog, feed then reset)"（对应宏
 * CONFIG_DEMO_WDT），app_main() 就会调用 bsp_wdt_demo()。
 */

#include "bsp_timer.h"
#include "board.h"
#include "bsp_led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 日志标签：本示例打印的日志会显示为 "wdt_demo" */
static const char *TAG = "wdt_demo";

void bsp_wdt_demo(void)
{
    /* 打印示例说明：先喂狗 5 次，之后停止喂狗以触发复位 */
    ESP_LOGI(TAG, "WDT demo start: feed 5 times, then stop to trigger reset");
    /* 初始化 LED，便于观察喂狗期间的心跳 */
    ESP_ERROR_CHECK(bsp_led_init());
    /* 初始化任务看门狗，并把当前任务加入监视（此后必须周期喂狗） */
    ESP_ERROR_CHECK(bsp_wdt_init());

    for (int i = 1; i <= 5; i++) {
        bsp_wdt_feed();     /* 喂狗：重置看门狗计时，证明程序「活着」 */
        bsp_led_toggle();   /* 翻转 LED，作为喂狗节拍的可视指示 */
        ESP_LOGI(TAG, "feed watchdog %d/5", i);   /* 打印当前是第几次喂狗 */
        vTaskDelay(pdMS_TO_TICKS(1000));          /* 间隔 1 秒喂一次，远小于 3s 超时 */
    }

    /* 警告：接下来不再喂狗，约 BSP_WDT_TIMEOUT_MS 后看门狗将触发复位 */
    ESP_LOGW(TAG, "stop feeding watchdog! board will reset in ~%d ms", BSP_WDT_TIMEOUT_MS);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));   /* 不再喂狗，等待超时复位 */
    }
}
