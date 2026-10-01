/**
 * @file bsp_wifi.h
 * @brief WiFi 板级驱动接口（ESP32-S3 内置 WiFi）
 */
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 通用初始化：NVS + netif + 事件循环 + WiFi 驱动 */
esp_err_t bsp_wifi_init(void);

/** WiFi 扫描示例（第四十八章） */
void bsp_wifi_scan_demo(void);

/** WiFi STA 连接示例（第四十九章） */
void bsp_wifi_sta_demo(void);

/** WiFi AP 热点示例（第五十章） */
void bsp_wifi_ap_demo(void);

/** WiFi 一键配网 SmartConfig 示例（第五十一章） */
void bsp_wifi_smartconfig_demo(void);

#ifdef __cplusplus
}
#endif
