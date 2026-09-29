#include "bsp_key.h"
#include "bsp_led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "key_demo";

void bsp_key_demo(void)
{
    ESP_LOGI(TAG, "KEY demo start: press BOOT key to toggle LED");
    ESP_ERROR_CHECK(bsp_key_init());
    ESP_ERROR_CHECK(bsp_led_init());

    while (1) {
        if (bsp_key_scan(0)) {
            bsp_led_toggle();
            ESP_LOGI(TAG, "key pressed, LED toggled");
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
