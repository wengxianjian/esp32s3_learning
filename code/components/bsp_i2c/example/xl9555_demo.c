#include "bsp_i2c.h"
#include "board.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "xl9555_demo";

void bsp_xl9555_demo(void)
{
    ESP_LOGI(TAG, "XL9555 demo start: toggle buzzer via I2C");
    ESP_ERROR_CHECK(bsp_i2c_init());
    ESP_ERROR_CHECK(bsp_xl9555_init());

    bool beep = false;
    while (1) {
        beep = !beep;
        bsp_xl9555_pin_write(BSP_XL9555_BEEP_IO, beep);
        ESP_LOGI(TAG, "buzzer %s", beep ? "ON" : "OFF");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
