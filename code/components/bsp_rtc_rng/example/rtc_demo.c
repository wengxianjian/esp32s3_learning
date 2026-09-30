#include "bsp_rtc_rng.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "rtc_demo";

void bsp_rtc_demo(void)
{
    ESP_LOGI(TAG, "RTC demo start");
    ESP_ERROR_CHECK(bsp_rtc_set_time(2024, 1, 1, 0, 0, 0));

    char buf[64];
    struct tm dt;

    while (1) {
        bsp_rtc_get_time(&dt);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &dt);
        ESP_LOGI(TAG, "time: %s", buf);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
