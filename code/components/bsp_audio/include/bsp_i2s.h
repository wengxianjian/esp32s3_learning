/**
 * @file bsp_i2s.h
 * @brief I2S 音频接口板级驱动接口（I2S0，主模式）
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 初始化 I2S0（主模式，16bit，44100Hz 立体声） */
esp_err_t bsp_i2s_init(void);

/** 写音频数据到 I2S 发送缓冲区 */
size_t bsp_i2s_write(const uint8_t *buf, size_t len);

/** 音频示例：ES8388 + I2S 播放 1kHz 正弦音 */
void bsp_audio_demo(void);

#ifdef __cplusplus
}
#endif
