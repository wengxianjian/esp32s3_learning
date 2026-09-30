/**
 * @file bsp_adc.h
 * @brief ADC 板级驱动接口（ADC1 通道 7 = GPIO8）
 *
 * 单元/通道/衰减由 board.h 中的 BSP_ADC_* 宏决定。
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 初始化 ADC1 单元与通道 */
esp_err_t bsp_adc_init(void);

/** 读取原始值（12 位：0~4095） */
int bsp_adc_read_raw(void);

/** 读取并换算为电压（mV，11dB 衰减近似线性换算） */
uint32_t bsp_adc_read_mv(void);

/** ADC 示例：周期性打印原始值与电压 */
void bsp_adc_demo(void);

#ifdef __cplusplus
}
#endif
