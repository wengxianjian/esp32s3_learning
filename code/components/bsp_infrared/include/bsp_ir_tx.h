/**
 * @file bsp_ir_tx.h
 * @brief 红外发送（RMT TX + NEC 编码）板级驱动接口
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 创建 RMT TX 通道 + 38kHz 载波 + NEC 编码器 */
esp_err_t bsp_ir_tx_init(void);

/** 发送一帧 NEC 信号 */
esp_err_t bsp_ir_tx_send(uint16_t address, uint16_t command);

/** 红外发送示例 */
void bsp_ir_tx_demo(void);

#ifdef __cplusplus
}
#endif
