/**
 * @file audio_demo.c
 * @brief 音频播放实验示例（对应指南第四十一章 音乐播放器）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 先初始化 ES8388（I2C0 配置编解码器）与 I2S0（音频数据通路）；
 * 2) 把耳机音量设为 20；
 * 3) 在死循环里用 sinf() 实时生成 1kHz 正弦波（16bit 立体声），
 *    不断写入 I2S 播放出去。
 * 演示现象：串口打印 "play 1kHz tone via I2S -> ES8388"，
 * 插上耳机即可听到持续的单音（1kHz 蜂鸣）。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "AUDIO" 这个 demo（对应宏 CONFIG_DEMO_AUDIO），
 * app_main() 就会调用 bsp_audio_demo()。
 * 硬件：需把音频模块插在板载音频接口，并插入耳机（或接喇叭）收听。
 */

#include "bsp_es8388.h"
#include "bsp_i2s.h"
#include "board.h"
#include "esp_log.h"
#include <math.h>

/* 日志标签：本示例打印的日志会显示为 "audio_demo" */
static const char *TAG = "audio_demo";

void bsp_audio_demo(void)
{
    /* 提示示例开始 */
    ESP_LOGI(TAG, "Audio demo start");
    /* 先初始化编解码器（配好音量/通路），再初始化 I2S（数据通路）。
     * ESP_ERROR_CHECK：失败即打印断言并复位，避免带病运行。 */
    ESP_ERROR_CHECK(bsp_es8388_init());
    ESP_ERROR_CHECK(bsp_i2s_init());
    /* 设置耳机音量：20（0~33 档，中等偏上，避免太吵） */
    bsp_es8388_set_hp_volume(20);
    ESP_LOGI(TAG, "play 1kHz tone via I2S -> ES8388");

    /* ── 生成 1kHz 正弦波（连续相位，16bit 立体声）──────────────
     * block：每批生成的采样点数（单声道计数）；buf 长度 = block × 2，
     *        因为立体声每点包含「左 + 右」两个 int16。 */
    const int block = 512;
    int16_t buf[block * 2];
    /* phase：当前相位（弧度）；float 保证相位连续、避免累积误差 */
    float phase = 0;
    /* step：每个采样的相位增量 = 2π × 目标频率 / 采样率
     *       = 2π × 1000 / 44100，即产生 1kHz 正弦波 */
    const float step = 2.0f * M_PI * 1000.0f / BSP_AUDIO_SAMPLE_RATE;

    while (1) {
        /* 逐点填充本批缓冲：每点算出一个采样值，左右声道写相同值（单声道内容立体声播放） */
        for (int i = 0; i < block; i++) {
            /* sinf(phase) 归一化到 [-1,1]，乘 8000 作振幅
             * （远小于 int16 上限 32767，避免削顶失真、音量适中） */
            int16_t s = (int16_t)(sinf(phase) * 8000.0f);
            /* 相位推进一个 step */
            phase += step;
            /* 相位超过 2π 就回绕，保持相位始终在 [0, 2π) 内，防止浮点精度损失 */
            if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
            buf[i * 2] = s;       /* 左声道 */
            buf[i * 2 + 1] = s;   /* 右声道 */
        }
        /* 把整批 PCM 数据（sizeof(buf) 字节）写入 I2S 播放；
         * 本调用会阻塞，直到数据被 DMA 取走，随后继续生成下一批，形成连续声音。 */
        bsp_i2s_write((const uint8_t *)buf, sizeof(buf));
    }
}
