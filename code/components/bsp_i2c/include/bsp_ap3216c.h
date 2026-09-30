/**
 * @file bsp_ap3216c.h
 * @brief AP3216C 光强(ALS)/接近(PS)/红外(IR)传感器板级驱动接口（I2C0）
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 复位 + 使能 ALS/PS/IR + 回读校验 */
esp_err_t bsp_ap3216c_init(void);

/** 读取三路数据（读数间隔需 > 112.5ms） */
esp_err_t bsp_ap3216c_read(uint16_t *ir, uint16_t *als, uint16_t *ps);

/** AP3216C 示例：周期读取并打印 */
void bsp_ap3216c_demo(void);

#ifdef __cplusplus
}
#endif
