#include "bsp_timer.h"
#include "board.h"
#include "bsp_led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "esptimer_demo";

void bsp_esptimer_demo(void)
{
    ESP_LOGI(TAG, "ESPTimer demo start: toggle LED every %d ms", BSP_ESPTIMER_PERIOD_US / 1000);
    ESP_ERROR_CHECK(bsp_led_init());
    ESP_ERROR_CHECK(bsp_esptimer_init());

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        ESP_LOGI(TAG, "timer fired %lu times", (unsigned long)bsp_esptimer_get_tick_count());
    }
}
