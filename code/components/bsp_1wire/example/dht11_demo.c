/**
 * @file dht11_demo.c
 * @brief DHT11 温湿度传感器实验示例（对应指南第三十章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 初始化单总线 DQ 引脚（IO0，开漏 + 上拉）并做一次应答自检；
 * 2) 上电后先等 1 秒（DHT11 上电稳定时间），再进入循环；
 * 3) 循环每 2 秒读一次温湿度并打印（整数：温度 °C、湿度 %RH）；
 * 4) 读取失败（无应答/校验错，通常是没插跳线帽或没接 DHT11）则不打印。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "1-Wire DHT11 (temperature + humidity)" 这个 demo
 * （对应宏 CONFIG_DEMO_DHT11），app_main() 就会调用 bsp_dht11_demo()。
 * 硬件：需用跳线帽把 WIRE_DQ 接到 IO0，并接上 DHT11 模块。
 */

#include "bsp_dht11.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 日志标签：本示例打印的日志会显示为 "dht11_demo" */
static const char *TAG = "dht11_demo";

void bsp_dht11_demo(void)
{
    /* 提示开始 */
    ESP_LOGI(TAG, "DHT11 demo start");
    /* 初始化 DQ + 自检。这里不用 ESP_ERROR_CHECK：未接传感器时只警告不中止，
     * 便于先跑通再插模块（打印提示检查跳线帽与模块）。 */
    if (bsp_dht11_init() != ESP_OK) {
        ESP_LOGW(TAG, "DHT11 init fail (check jumper cap + sensor)");
    }
    /* 上电后至少等 1s：DHT11 上电初期不稳定，过早读会失败 */
    vTaskDelay(pdMS_TO_TICKS(1000));   /* 上电后至少等 1s */

    /* 出参缓冲：temp=温度(°C)，humi=湿度(%RH)，读到后由 bsp_dht11_read 写入 */
    uint8_t temp, humi;
    while (1) {
        /* 读一次温湿度；仅在校验通过（ESP_OK）时打印 */
        if (bsp_dht11_read(&temp, &humi) == ESP_OK) {
            /* %% 转义为字面百分号，%d 打印整数温度/湿度 */
            ESP_LOGI(TAG, "temp = %d C, humi = %d %%", temp, humi);
        }
        /* 每 2 秒读一次：DHT11 读数间隔需 > 1s，2s 更保险 */
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
