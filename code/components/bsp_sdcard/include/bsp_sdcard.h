/**
 * @file bsp_sdcard.h
 * @brief SD 卡（SPI + FAT 文件系统）板级驱动接口
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 初始化 SPI2 并挂载 SD 卡 FAT 文件系统 */
esp_err_t bsp_sdcard_init(void);

/** SD 卡示例：挂载 + 容量查询 + 文件读写校验 */
void bsp_sdcard_demo(void);

#ifdef __cplusplus
}
#endif
