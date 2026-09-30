/**
 * @file bsp_eeprom.h
 * @brief AT24C02 EEPROM 板级驱动接口（I2C）
 *
 * 地址由 board.h 中的 BSP_EEPROM_ADDR 决定。
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 在指定地址写入数据 */
esp_err_t bsp_eeprom_write(uint16_t addr, const uint8_t *buf, uint16_t len);

/** 从指定地址读取数据 */
esp_err_t bsp_eeprom_read(uint16_t addr, uint8_t *buf, uint16_t len);

/** EEPROM 示例：写一段数据并读回校验 */
void bsp_eeprom_demo(void);

#ifdef __cplusplus
}
#endif
