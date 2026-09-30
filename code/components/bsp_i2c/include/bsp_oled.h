/**
 * @file bsp_oled.h
 * @brief SSD1306 OLED 板级驱动接口（I2C1，128x64）
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 初始化 I2C1 + DC 引脚 + SSD1306 */
esp_err_t bsp_oled_init(void);

/** 清屏 */
esp_err_t bsp_oled_clear(void);

/** 在指定位置显示字符串（8x16 字体，x=列 0~127，y=起始页，取 0/2/4/6 表示 4 行） */
void bsp_oled_show_string(uint8_t x, uint8_t y, const char *str);

/** OLED 示例：显示标题与计数 */
void bsp_oled_demo(void);

#ifdef __cplusplus
}
#endif
