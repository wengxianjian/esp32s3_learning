/**
 * @file bsp_led.h
 * @brief LED 板级驱动接口（GPIO 输出）
 *
 * 引脚与电平由 board.h 中的 BSP_LED0_* 宏决定，本驱动不写死任何引脚号。
 */
#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 初始化 LED 对应 GPIO 为输出，并默认熄灭 */
esp_err_t bsp_led_init(void);

/** 点亮/熄灭 LED（on=true 点亮） */
esp_err_t bsp_led_set(bool on);

/** 翻转 LED 电平 */
esp_err_t bsp_led_toggle(void);

/** LED 示例：以 500ms 间隔闪烁 */
void bsp_led_demo(void);

#ifdef __cplusplus
}
#endif
