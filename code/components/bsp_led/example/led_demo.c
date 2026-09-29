#include "bsp_led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "led_demo";

void bsp_led_demo(void)
{
    ESP_LOGI(TAG, "LED demo start: blink every 500 ms");
    ESP_ERROR_CHECK(bsp_led_init());

    while (1) {
        bsp_led_set(true);
        vTaskDelay(pdMS_TO_TICKS(500));
        bsp_led_set(false);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
