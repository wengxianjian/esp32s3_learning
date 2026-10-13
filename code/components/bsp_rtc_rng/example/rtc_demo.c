/**
 * @file rtc_demo.c
 * @brief RTC 实验示例（对应指南第二十三章 RTC）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 先把系统时间设置为 2024-01-01 00:00:00；
 * 2) 进入死循环，每秒读取一次当前时间，格式化成 "YYYY-mm-dd HH:MM:SS" 打印；
 * 3) 可以看到时间每秒稳定递增 —— 说明系统时间由 RTC 硬件持续维持。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "RTC (set and read time)"（对应宏 CONFIG_DEMO_RTC），
 * app_main() 就会调用 bsp_rtc_demo()。
 */

#include "bsp_rtc_rng.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 日志标签：本示例打印的日志会显示为 "rtc_demo" */
static const char *TAG = "rtc_demo";

void bsp_rtc_demo(void)
{
    ESP_LOGI(TAG, "RTC demo start");
    /* 设置起始时间 2024-01-01 00:00:00（年/月/日/时/分/秒）。
     * 参数是常规的年月日数值，内部会自动处理 struct tm 的偏移（-1900、-1） */
    ESP_ERROR_CHECK(bsp_rtc_set_time(2024, 1, 1, 0, 0, 0));

    /* 字符串缓冲，用于存放格式化后的时间文本（如 "2024-01-01 00:00:00"，19 字符 + '\0'） */
    char buf[64];
    /* struct tm 时间结构体，由 bsp_rtc_get_time() 填充 */
    struct tm dt;

    while (1) {
        /* 读取当前系统时间到 dt */
        bsp_rtc_get_time(&dt);
        /* 用 strftime 按 "%Y-%m-%d %H:%M:%S" 格式写入 buf，便于直观查看 */
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &dt);
        /* 打印当前时间字符串，正常情况下每秒递增 1 秒 */
        ESP_LOGI(TAG, "time: %s", buf);
        /* 每隔 1 秒读取打印一次 */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
