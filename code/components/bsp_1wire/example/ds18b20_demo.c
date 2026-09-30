#include "bsp_ds18b20.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "ds18b20_demo";

void bsp_ds18b20_demo(void)
{
    ESP_LOGI(TAG, "DS18B20 demo start");
    ESP_ERROR_CHECK(bsp_ds18b20_init());

    while (1) {
        float temp = bsp_ds18b20_read_temperature();
        if (!isnan(temp)) {
            ESP_LOGI(TAG, "temperature = %.2f C", temp);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
