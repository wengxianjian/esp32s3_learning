#include "bsp_adc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "adc_demo";

void bsp_adc_demo(void)
{
    ESP_LOGI(TAG, "ADC demo start: read ADC1_CHANNEL_7 (GPIO8)");
    ESP_ERROR_CHECK(bsp_adc_init());

    while (1) {
        int raw = bsp_adc_read_raw();
        uint32_t mv = bsp_adc_read_mv();
        ESP_LOGI(TAG, "raw = %d, voltage = %lu mV", raw, (unsigned long)mv);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
