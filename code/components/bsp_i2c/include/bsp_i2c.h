/**
 * @file bsp_i2c.h
 * @brief I2C0 总线 + XL9555 IO 扩展 板级驱动接口
 *
 * 引脚/频率/地址由 board.h 中的 BSP_I2C0_* / BSP_XL9555_* 宏决定。
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- I2C0 总线 ---- */
esp_err_t bsp_i2c_init(void);
esp_err_t bsp_i2c_write_reg(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len);
esp_err_t bsp_i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *data, size_t len);

/* ---- XL9555 ---- */
esp_err_t bsp_xl9555_init(void);
void bsp_xl9555_pin_write(uint16_t pin, int val);
int bsp_xl9555_pin_read(uint16_t pin);
void bsp_xl9555_demo(void);

#ifdef __cplusplus
}
#endif
