/**
 * @file bsp_qma6100p.h
 * @brief QMA6100P 三轴加速度传感器板级驱动接口（I2C0）
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 复位 + 配置 + 校验 CHIP_ID */
esp_err_t bsp_qma6100p_init(void);

/** 读取三轴原始数据（14 位有符号） */
esp_err_t bsp_qma6100p_read_raw(int16_t *x, int16_t *y, int16_t *z);

/** QMA6100P 示例：周期读取并打印三轴 + 俯仰/翻滚角 */
void bsp_qma6100p_demo(void);

#ifdef __cplusplus
}
#endif
