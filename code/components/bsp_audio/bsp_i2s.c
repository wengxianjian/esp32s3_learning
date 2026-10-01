#include "bsp_i2s.h"
#include "board.h"
#include "driver/i2s.h"
#include "esp_log.h"

static const char *TAG = "bsp_i2s";

esp_err_t bsp_i2s_init(void)
{
    i2s_config_t cfg = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX,
        .sample_rate = BSP_AUDIO_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
    };
    esp_err_t ret = i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    i2s_pin_config_t pin = {
        .bck_io_num = BSP_I2S_BCK_GPIO,
        .ws_io_num = BSP_I2S_WS_GPIO,
        .data_out_num = BSP_I2S_DO_GPIO,
        .data_in_num = BSP_I2S_DI_GPIO,
        .mck_io_num = BSP_I2S_MCLK_GPIO,
    };
    ret = i2s_set_pin(I2S_NUM_0, &pin);
    if (ret != ESP_OK) return ret;

    i2s_zero_dma_buffer(I2S_NUM_0);
    ESP_LOGI(TAG, "I2S0 init OK (%d Hz)", BSP_AUDIO_SAMPLE_RATE);
    return ESP_OK;
}

size_t bsp_i2s_write(const uint8_t *buf, size_t len)
{
    size_t written = 0;
    i2s_write(I2S_NUM_0, buf, len, &written, portMAX_DELAY);
    return written;
}
