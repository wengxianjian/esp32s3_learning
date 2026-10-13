/**
 * @file tsens_demo.c
 * @brief 内部温度传感器实验示例（对应指南第二十八章 内部温度传感器实验）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 安装芯片内部温度传感器（量程 -10 ~ 80 ℃）；
 * 2) 进入死循环，每秒读取一次芯片结温并打印（保留两位小数）；
 * 3) 观察到的是一个稳定的温度值（通常略高于环境温度），用手按住芯片可看到上升。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "TSENS (internal temperature sensor)"（对应宏
 * CONFIG_DEMO_TSENS），app_main() 就会调用 bsp_tsens_demo()。
 */

#include "bsp_rtc_rng.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 日志标签：本示例打印的日志会显示为 "tsens_demo" */
static const char *TAG = "tsens_demo";

void bsp_tsens_demo(void)
{
    ESP_LOGI(TAG, "TSENS demo start");
    /* 安装内部温度传感器（配置量程/时钟源），失败即断言复位；只需安装一次 */
    ESP_ERROR_CHECK(bsp_tsens_init());

    while (1) {
        /* 读取一次芯片温度（内部会 enable -> 读取 -> disable 以省电） */
        float t = bsp_tsens_read_celsius();
        /* 打印温度，%.2f 保留两位小数（单位：摄氏度） */
        ESP_LOGI(TAG, "temperature = %.2f C", t);
        /* 每秒读取一次 */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
