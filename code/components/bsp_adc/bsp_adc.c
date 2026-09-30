#include "bsp_adc.h"
#include "board.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "bsp_adc";
static adc_oneshot_unit_handle_t s_adc = NULL;

esp_err_t bsp_adc_init(void)
{
    adc_oneshot_unit_init_cfg_t cfg = {
        .unit_id = BSP_ADC_UNIT,
    };
    esp_err_t ret = adc_oneshot_new_unit(&cfg, &s_adc);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %s", esp_err_to_name(ret));
        return ret;
    }

    adc_oneshot_chan_cfg_t chan = {
        .atten = BSP_ADC_ATTEN,
        .bitwidth = BSP_ADC_BITWIDTH,
    };
    ret = adc_oneshot_config_channel(s_adc, BSP_ADC_CHANNEL, &chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_config_channel failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "ADC1 channel %d initialized", BSP_ADC_CHANNEL);
    return ESP_OK;
}

int bsp_adc_read_raw(void)
{
    int raw = 0;
    adc_oneshot_read(s_adc, BSP_ADC_CHANNEL, &raw);
    return raw;
}

uint32_t bsp_adc_read_mv(void)
{
    int raw = bsp_adc_read_raw();
    /* 11dB 衰减量程约 0~3100mV，12 位 0~4095，近似线性换算 */
    return (uint32_t)(raw * 3100 / 4095);
}
