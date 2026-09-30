#include "bsp_eeprom.h"
#include "bsp_i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "eeprom_demo";

void bsp_eeprom_demo(void)
{
    ESP_LOGI(TAG, "EEPROM demo start");
    ESP_ERROR_CHECK(bsp_i2c_init());

    const uint8_t write_data[] = "ESP32S3";   /* 8 字节，正好一页 */
    uint8_t read_data[sizeof(write_data)] = {0};

    ESP_ERROR_CHECK(bsp_eeprom_write(0, write_data, sizeof(write_data)));
    ESP_ERROR_CHECK(bsp_eeprom_read(0, read_data, sizeof(write_data)));

    ESP_LOGI(TAG, "written:  %s", write_data);
    ESP_LOGI(TAG, "readback: %s", read_data);
    if (memcmp(write_data, read_data, sizeof(write_data)) == 0) {
        ESP_LOGI(TAG, "EEPROM read/write OK");
    } else {
        ESP_LOGE(TAG, "EEPROM read/write mismatch!");
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        bsp_eeprom_read(0, read_data, sizeof(write_data));
        ESP_LOGI(TAG, "persistent data: %s", read_data);
    }
}
