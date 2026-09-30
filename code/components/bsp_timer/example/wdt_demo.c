#include "bsp_timer.h"
#include "board.h"
#include "bsp_led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "wdt_demo";

void bsp_wdt_demo(void)
{
    ESP_LOGI(TAG, "WDT demo start: feed 5 times, then stop to trigger reset");
    ESP_ERROR_CHECK(bsp_led_init());
    ESP_ERROR_CHECK(bsp_wdt_init());

    for (int i = 1; i <= 5; i++) {
        bsp_wdt_feed();
        bsp_led_toggle();
        ESP_LOGI(TAG, "feed watchdog %d/5", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGW(TAG, "stop feeding watchdog! board will reset in ~%d ms", BSP_WDT_TIMEOUT_MS);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));   /* 不再喂狗，等待超时复位 */
    }
}
