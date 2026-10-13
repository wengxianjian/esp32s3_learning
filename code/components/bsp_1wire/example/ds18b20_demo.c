/**
 * @file ds18b20_demo.c
 * @brief DS18B20 温度传感器实验示例（对应指南第二十九章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 初始化单总线 DQ 引脚（IO0，开漏 + 上拉）；
 * 2) 循环每 1 秒读一次温度并打印（单位 °C，保留两位小数）；
 * 3) 若读数失败（返回 NAN，通常是没插跳线帽或没接 DS18B20），则跳过本次打印。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "1-Wire DS18B20 (temperature)" 这个 demo
 * （对应宏 CONFIG_DEMO_DS18B20），app_main() 就会调用 bsp_ds18b20_demo()。
 * 硬件：需用跳线帽把 WIRE_DQ 接到 IO0，并接上 DS18B20 模块。
 */

#include "bsp_ds18b20.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

/* 日志标签：本示例打印的日志会显示为 "ds18b20_demo" */
static const char *TAG = "ds18b20_demo";

void bsp_ds18b20_demo(void)
{
    /* 提示开始 */
    ESP_LOGI(TAG, "DS18B20 demo start");
    /* 初始化 DQ 引脚。ESP_ERROR_CHECK：失败（返回非 ESP_OK）会打印断言并复位 */
    ESP_ERROR_CHECK(bsp_ds18b20_init());

    while (1) {
        /* 读一次温度（内部含转换 + 750ms 等待，无器件时返回 NAN） */
        float temp = bsp_ds18b20_read_temperature();
        /* isnan：只有读到有效值才打印，避免刷屏一堆 nan */
        if (!isnan(temp)) {
            ESP_LOGI(TAG, "temperature = %.2f C", temp);
        }
        /* 每 1 秒读一次（1000 个 tick；默认 1 tick = 1ms） */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
