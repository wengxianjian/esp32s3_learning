#include "bsp_usb.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "usb_msc_demo";

void bsp_usb_msc_demo(void)
{
    ESP_LOGI(TAG, "USB MSC demo start");
    if (bsp_usb_msc_init() != ESP_OK) {
        ESP_LOGE(TAG, "USB MSC init failed");
        return;
    }
    ESP_LOGI(TAG, "Flash U-disk ready, connect to PC via USB");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
