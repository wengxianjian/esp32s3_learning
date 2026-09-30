#include "bsp_dht11.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "dht11_demo";

void bsp_dht11_demo(void)
{
    ESP_LOGI(TAG, "DHT11 demo start");
    if (bsp_dht11_init() != ESP_OK) {
        ESP_LOGW(TAG, "DHT11 init fail (check jumper cap + sensor)");
    }
    vTaskDelay(pdMS_TO_TICKS(1000));   /* 上电后至少等 1s */

    uint8_t temp, humi;
    while (1) {
        if (bsp_dht11_read(&temp, &humi) == ESP_OK) {
            ESP_LOGI(TAG, "temp = %d C, humi = %d %%", temp, humi);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
