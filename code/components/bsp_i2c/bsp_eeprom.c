#include "bsp_eeprom.h"
#include "bsp_i2c.h"
#include "board.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "bsp_eeprom";

esp_err_t bsp_eeprom_write(uint16_t addr, const uint8_t *buf, uint16_t len)
{
    esp_err_t ret = bsp_i2c_write_reg(BSP_EEPROM_ADDR, (uint8_t)addr, buf, len);
    vTaskDelay(pdMS_TO_TICKS(10));   /* 等待 EEPROM 内部写入周期 */
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "write failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t bsp_eeprom_read(uint16_t addr, uint8_t *buf, uint16_t len)
{
    esp_err_t ret = bsp_i2c_read_reg(BSP_EEPROM_ADDR, (uint8_t)addr, buf, len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "read failed: %s", esp_err_to_name(ret));
    }
    return ret;
}
