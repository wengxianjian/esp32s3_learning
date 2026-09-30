/**
 * @file bsp_dht11.h
 * @brief DHT11 单总线温湿度传感器板级驱动接口
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 初始化单总线 GPIO 并复位 + 自检 */
esp_err_t bsp_dht11_init(void);

/** 读取温湿度（整数部分），读数间隔需 > 1s */
esp_err_t bsp_dht11_read(uint8_t *temp, uint8_t *humi);

/** DHT11 示例：周期读取并打印温湿度 */
void bsp_dht11_demo(void);

#ifdef __cplusplus
}
#endif
