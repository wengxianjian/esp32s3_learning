/**
 * @file bsp_es8388.h
 * @brief ES8388 音频编解码器板级驱动接口（I2C0 控制）
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 初始化 ES8388（复位 + 完整配置序列） */
esp_err_t bsp_es8388_init(void);

/** 设置耳机音量（0~33） */
void bsp_es8388_set_hp_volume(uint8_t volume);

/** 设置喇叭音量（0~33） */
void bsp_es8388_set_spk_volume(uint8_t volume);

#ifdef __cplusplus
}
#endif
