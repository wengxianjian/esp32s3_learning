#include "bsp_rtc_rng.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "tsens_demo";

void bsp_tsens_demo(void)
{
    ESP_LOGI(TAG, "TSENS demo start");
    ESP_ERROR_CHECK(bsp_tsens_init());

    while (1) {
        float t = bsp_tsens_read_celsius();
        ESP_LOGI(TAG, "temperature = %.2f C", t);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
