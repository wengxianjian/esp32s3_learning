/**
 * @file eeprom_demo.c
 * @brief AT24C02 EEPROM 读写实验示例（对应指南第二十章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 初始化 I2C0；
 * 2) 从存储地址 0 写入字符串 "ESP32S3"（8 字节，正好一页）；
 * 3) 从地址 0 读回并与写入内容比对，打印 written / readback 及校验结果；
 * 4) 之后每隔 3 秒再读一次，用来观察数据在复位/断电后仍能保持（掉电不丢失）。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * menuconfig 选 "EEPROM"（宏 CONFIG_DEMO_EEPROM），
 * app_main() 调用 bsp_eeprom_demo()。
 */

#include "bsp_eeprom.h"
#include "bsp_i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/* 日志标签：本示例日志显示为 "eeprom_demo" */
static const char *TAG = "eeprom_demo";

/**
 * @brief EEPROM 实验入口：写一段数据并读回校验，之后周期回读
 *
 * 逻辑流程：
 *   ① 初始化 I2C0
 *   ② 从地址 0 写入 8 字节 "ESP32S3"
 *   ③ 从地址 0 读回并 memcmp 比对，打印结果
 *   ④ 死循环：每 3 秒回读一次，演示掉电不丢失
 */
void bsp_eeprom_demo(void)
{
    ESP_LOGI(TAG, "EEPROM demo start");
    /* 初始化 I2C0（AT24C02 挂在 I2C0 上） */
    ESP_ERROR_CHECK(bsp_i2c_init());

    const uint8_t write_data[] = "ESP32S3";   /* 8 字节，正好一页 */
    uint8_t read_data[sizeof(write_data)] = {0};   /* 与写入等长的读缓冲，初值全 0 */

    /* 从地址 0 写入，再从地址 0 读回。
     * 注意：AT24C02 页大小 8 字节、跨页写需分段，这里 8 字节恰好一页，是安全的。 */
    ESP_ERROR_CHECK(bsp_eeprom_write(0, write_data, sizeof(write_data)));
    ESP_ERROR_CHECK(bsp_eeprom_read(0, read_data, sizeof(write_data)));

    ESP_LOGI(TAG, "written:  %s", write_data);
    ESP_LOGI(TAG, "readback: %s", read_data);
    /* memcmp 返回 0 表示写入与读回完全一致 */
    if (memcmp(write_data, read_data, sizeof(write_data)) == 0) {
        ESP_LOGI(TAG, "EEPROM read/write OK");
    } else {
        ESP_LOGE(TAG, "EEPROM read/write mismatch!");
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(3000));    /* 每 3 秒回读一次 */
        /* 重新读回并打印：可断电重启后观察数据是否仍在（EEPROM 掉电不丢失） */
        bsp_eeprom_read(0, read_data, sizeof(write_data));
        ESP_LOGI(TAG, "persistent data: %s", read_data);
    }
}
