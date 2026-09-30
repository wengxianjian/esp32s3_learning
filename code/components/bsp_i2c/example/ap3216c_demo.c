#include "bsp_ap3216c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ap3216c_demo";

void bsp_ap3216c_demo(void)
{
    ESP_LOGI(TAG, "AP3216C demo start");
    ESP_ERROR_CHECK(bsp_ap3216c_init());

    uint16_t ir, als, ps;
    while (1) {
        ESP_ERROR_CHECK(bsp_ap3216c_read(&ir, &als, &ps));
        ESP_LOGI(TAG, "IR=%u  ALS=%u  PS=%u", ir, als, ps);
        vTaskDelay(pdMS_TO_TICKS(500));   /* 大于 112.5ms 的转换间隔 */
    }
}
