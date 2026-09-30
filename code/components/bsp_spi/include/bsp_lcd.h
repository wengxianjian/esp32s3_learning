/**
 * @file bsp_lcd.h
 * @brief SPI LCD（ILI9341，320x240）板级驱动接口
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* RGB565 常用色 */
#define LCD_COLOR_RED      0xF800
#define LCD_COLOR_GREEN    0x07E0
#define LCD_COLOR_BLUE     0x001F
#define LCD_COLOR_WHITE    0xFFFF
#define LCD_COLOR_BLACK    0x0000
#define LCD_COLOR_YELLOW   0xFFE0
#define LCD_COLOR_CYAN     0x07FF
#define LCD_COLOR_MAGENTA  0xF81F

/** 初始化 SPI2 + ILI9341 + 背光 */
esp_err_t bsp_lcd_init(void);

/** 整屏填充指定颜色 */
void bsp_lcd_fill(uint16_t color);

/** 填充指定矩形区域 */
void bsp_lcd_fill_rect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

/** SPI LCD 示例：循环切换背景色 */
void bsp_lcd_demo(void);

#ifdef __cplusplus
}
#endif
