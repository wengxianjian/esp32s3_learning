#include "bsp_oled.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

static const char *TAG = "oled_demo";

void bsp_oled_demo(void)
{
    ESP_LOGI(TAG, "OLED demo start");
    ESP_ERROR_CHECK(bsp_oled_init());

    bsp_oled_show_string(0, 0, "ESP32-S3 OLED");
    bsp_oled_show_string(0, 2, "ATK-DNESP32S3");
    bsp_oled_show_string(0, 4, "I2C Test");

    uint32_t cnt = 0;
    char buf[32];
    while (1) {
        snprintf(buf, sizeof(buf), "cnt:%lu", (unsigned long)cnt++);
        bsp_oled_show_string(0, 6, buf);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
