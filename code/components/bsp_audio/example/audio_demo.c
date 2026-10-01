#include "bsp_es8388.h"
#include "bsp_i2s.h"
#include "board.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "audio_demo";

void bsp_audio_demo(void)
{
    ESP_LOGI(TAG, "Audio demo start");
    ESP_ERROR_CHECK(bsp_es8388_init());
    ESP_ERROR_CHECK(bsp_i2s_init());
    bsp_es8388_set_hp_volume(20);
    ESP_LOGI(TAG, "play 1kHz tone via I2S -> ES8388");

    /* 生成 1kHz 正弦波（连续相位，16bit 立体声） */
    const int block = 512;
    int16_t buf[block * 2];
    float phase = 0;
    const float step = 2.0f * M_PI * 1000.0f / BSP_AUDIO_SAMPLE_RATE;

    while (1) {
        for (int i = 0; i < block; i++) {
            int16_t s = (int16_t)(sinf(phase) * 8000.0f);
            phase += step;
            if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
            buf[i * 2] = s;       /* 左声道 */
            buf[i * 2 + 1] = s;   /* 右声道 */
        }
        bsp_i2s_write((const uint8_t *)buf, sizeof(buf));
    }
}
