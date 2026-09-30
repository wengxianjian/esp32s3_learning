#include "bsp_qma6100p.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "qma6100p_demo";

void bsp_qma6100p_demo(void)
{
    ESP_LOGI(TAG, "QMA6100P demo start");
    ESP_ERROR_CHECK(bsp_qma6100p_init());

    int16_t x, y, z;
    while (1) {
        ESP_ERROR_CHECK(bsp_qma6100p_read_raw(&x, &y, &z));
        float pitch = atan2f(x, sqrtf((float)y * y + (float)z * z)) * 180.0f / M_PI;
        float roll  = atan2f(y, sqrtf((float)x * x + (float)z * z)) * 180.0f / M_PI;
        ESP_LOGI(TAG, "X=%d Y=%d Z=%d | pitch=%.1f roll=%.1f", x, y, z, pitch, roll);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}
