#include "bsp_key.h"
#include "bsp_led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "exit_demo";

void bsp_exit_demo(void)
{
    ESP_LOGI(TAG, "EXIT demo start: press BOOT key (interrupt) to toggle LED");
    ESP_ERROR_CHECK(bsp_key_intr_init());
    ESP_ERROR_CHECK(bsp_led_init());

    while (1) {
        if (bsp_key_intr_triggered()) {
            bsp_led_toggle();
            ESP_LOGI(TAG, "key interrupt triggered, LED toggled");
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
