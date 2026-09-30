#include "bsp_timer.h"
#include "board.h"
#include "bsp_led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "gptimer_demo";

void bsp_gptimer_demo(void)
{
    ESP_LOGI(TAG, "GPTimer demo start: toggle LED every %d ms", BSP_GPTIMER_PERIOD_US / 1000);
    ESP_ERROR_CHECK(bsp_led_init());
    ESP_ERROR_CHECK(bsp_gptimer_init());

    uint32_t last_tick = 0;
    TickType_t last_log = xTaskGetTickCount();

    while (1) {
        uint32_t tick = bsp_gptimer_get_tick_count();
        if (tick != last_tick) {
            bsp_led_toggle();
            last_tick = tick;
        }
        if (xTaskGetTickCount() - last_log >= pdMS_TO_TICKS(2000)) {
            last_log = xTaskGetTickCount();
            ESP_LOGI(TAG, "gptimer fired %lu times", (unsigned long)tick);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
