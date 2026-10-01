/**
 * @file bsp_usb.h
 * @brief USB MSC（模拟 U 盘）板级驱动接口（Flash FAT 分区）
 */
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 挂载 Flash FAT 分区并初始化为 USB 大容量存储设备 */
esp_err_t bsp_usb_msc_init(void);

/** USB MSC 示例 */
void bsp_usb_msc_demo(void);

#ifdef __cplusplus
}
#endif
