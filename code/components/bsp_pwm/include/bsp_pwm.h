/**
 * @file bsp_pwm.h
 * @brief PWM 板级驱动接口（LEDC，输出到 LED）
 *
 * 定时器/通道/频率由 board.h 中的 BSP_PWM_* 宏决定。
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 初始化 LEDC 定时器与通道（输出到 LED GPIO） */
esp_err_t bsp_pwm_init(void);

/** 软件方式设置占空比（0 ~ BSP_PWM_DUTY_MAX） */
esp_err_t bsp_pwm_set_duty(uint32_t duty);

/** 硬件渐变到目标占空比（time_ms 内完成） */
esp_err_t bsp_pwm_fade_to(uint32_t target_duty, int time_ms);

/** 软件 PWM 示例：循环改占空比实现呼吸灯 */
void bsp_swpwm_demo(void);

/** 硬件 PWM 示例：LEDC 渐变实现呼吸灯 */
void bsp_hwpwm_demo(void);

#ifdef __cplusplus
}
#endif
