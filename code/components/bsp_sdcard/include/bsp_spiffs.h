/**
 * @file bsp_spiffs.h
 * @brief SPIFFS 文件系统（内部 Flash）板级驱动接口
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 挂载 SPIFFS 文件系统（失败时自动格式化） */
esp_err_t bsp_spiffs_init(void);

/** SPIFFS 示例：挂载 + 容量查询 + 文件读写校验 */
void bsp_spiffs_demo(void);

#ifdef __cplusplus
}
#endif
