/**
 * @file bsp_timer.h
 * @brief 定时器板级驱动接口（ESPTimer 软定时器）
 *
 * 定时周期由 board.h 中的 BSP_ESPTIMER_* 宏决定。
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 创建并启动周期性高分辨率定时器（回调中翻转 LED） */
esp_err_t bsp_esptimer_init(void);

/** 获取定时器已触发的次数 */
uint32_t bsp_esptimer_get_tick_count(void);

/** ESPTimer 示例：定时器周期翻转 LED */
void bsp_esptimer_demo(void);

/** 创建并启动 GPTimer 硬件定时器（报警回调中计数） */
esp_err_t bsp_gptimer_init(void);

/** 获取 GPTimer 已触发的报警次数 */
uint32_t bsp_gptimer_get_tick_count(void);

/** GPTimer 示例：硬件定时器周期翻转 LED */
void bsp_gptimer_demo(void);

#ifdef __cplusplus
}
#endif
