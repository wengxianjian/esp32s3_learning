/**
 * @file bsp_rtc_rng.h
 * @brief RTC / RNG / 内部温度传感器 板级驱动接口（均为芯片内部资源）
 */
#pragma once

#include <stdint.h>
#include <time.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- RTC ---- */
esp_err_t bsp_rtc_set_time(int year, int mon, int mday, int hour, int min, int sec);
void bsp_rtc_get_time(struct tm *dt);
void bsp_rtc_demo(void);

/* ---- RNG ---- */
uint32_t bsp_rng_random(void);
int bsp_rng_range(int min, int max);
void bsp_rng_demo(void);

/* ---- 内部温度传感器 ---- */
esp_err_t bsp_tsens_init(void);
float bsp_tsens_read_celsius(void);
void bsp_tsens_demo(void);

#ifdef __cplusplus
}
#endif
