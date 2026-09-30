/**
 * @file bsp_ds18b20.h
 * @brief DS18B20 单总线温度传感器板级驱动接口
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 初始化单总线 GPIO（开漏 + 上拉） */
esp_err_t bsp_ds18b20_init(void);

/** 读取温度（摄氏度，分辨率 0.0625°C），失败返回 NaN */
float bsp_ds18b20_read_temperature(void);

/** DS18B20 示例：周期读取并打印温度 */
void bsp_ds18b20_demo(void);

#ifdef __cplusplus
}
#endif
