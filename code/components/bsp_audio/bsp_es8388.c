#include "bsp_es8388.h"
#include "bsp_i2c.h"
#include "board.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "bsp_es8388";

static esp_err_t es8388_write_reg(uint8_t reg, uint8_t val)
{
    return bsp_i2c_write_reg(BSP_ES8388_ADDR, reg, &val, 1);
}

esp_err_t bsp_es8388_init(void)
{
    esp_err_t ret = bsp_i2c_init();
    if (ret != ESP_OK) return ret;

    es8388_write_reg(0x00, 0x80);   /* 复位 */
    es8388_write_reg(0x00, 0x00);
    vTaskDelay(pdMS_TO_TICKS(100));

    es8388_write_reg(0x01, 0x58);
    es8388_write_reg(0x01, 0x50);
    es8388_write_reg(0x02, 0xF3);
    es8388_write_reg(0x02, 0xF0);
    es8388_write_reg(0x03, 0x09);   /* 麦克风偏置 */
    es8388_write_reg(0x00, 0x06);   /* 参考电压 */
    es8388_write_reg(0x04, 0x00);   /* DAC 电源管理 */
    es8388_write_reg(0x08, 0x00);   /* MCLK 分频 */
    es8388_write_reg(0x2B, 0x80);   /* DAC LRCK/ADC LRCK */
    es8388_write_reg(0x09, 0x88);   /* ADC PGA +24dB */
    es8388_write_reg(0x0C, 0x4C);   /* ADC 数据 16bit */
    es8388_write_reg(0x0D, 0x02);   /* ADC MCLK/rate=256 */
    es8388_write_reg(0x10, 0x00);   /* ADC 音量 L */
    es8388_write_reg(0x11, 0x00);   /* ADC 音量 R */
    es8388_write_reg(0x17, 0x18);   /* DAC 数据 16bit */
    es8388_write_reg(0x18, 0x02);   /* DAC MCLK/rate=256 */
    es8388_write_reg(0x1A, 0x00);   /* DAC 音量 L */
    es8388_write_reg(0x1B, 0x00);   /* DAC 音量 R */
    es8388_write_reg(0x27, 0xB8);   /* L 混频器 */
    es8388_write_reg(0x2A, 0xB8);   /* R 混频器 */
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_LOGI(TAG, "ES8388 init OK (addr 0x%02x)", BSP_ES8388_ADDR);
    return ESP_OK;
}

void bsp_es8388_set_hp_volume(uint8_t volume)
{
    if (volume > 33) volume = 33;
    es8388_write_reg(0x2E, volume);
    es8388_write_reg(0x2F, volume);
}

void bsp_es8388_set_spk_volume(uint8_t volume)
{
    if (volume > 33) volume = 33;
    es8388_write_reg(0x30, volume);
    es8388_write_reg(0x31, volume);
}
