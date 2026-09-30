#include "bsp_rtc_rng.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "rng_demo";

void bsp_rng_demo(void)
{
    ESP_LOGI(TAG, "RNG demo start");

    while (1) {
        uint32_t r = bsp_rng_random();
        int range = bsp_rng_range(1, 100);
        ESP_LOGI(TAG, "random = %lu, range[1,100] = %d", (unsigned long)r, range);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
