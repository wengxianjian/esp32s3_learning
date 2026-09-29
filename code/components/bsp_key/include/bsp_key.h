/**
 * @file bsp_key.h
 * @brief 独立按键（BOOT 键）板级驱动接口（GPIO 输入）
 *
 * 引脚与有效电平由 board.h 中的 BSP_KEY0_* 宏决定，本驱动不写死任何引脚号。
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 初始化按键 GPIO 为输入（带内部上拉） */
esp_err_t bsp_key_init(void);

/**
 * @brief 按键扫描（软件消抖 10ms）
 * @param mode 0=不支持连按（一次按下只返回一次）；1=支持连按（按住期间每次调用都返回）
 * @return 0=无按键；1=检测到按下
 */
uint8_t bsp_key_scan(uint8_t mode);

/** KEY 示例：按下 BOOT 键翻转 LED */
void bsp_key_demo(void);

#ifdef __cplusplus
}
#endif
