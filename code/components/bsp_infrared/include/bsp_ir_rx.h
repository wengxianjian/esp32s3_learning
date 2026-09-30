/**
 * @file bsp_ir_rx.h
 * @brief 红外接收（RMT RX + NEC 解码）板级驱动接口
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 创建 RMT RX 通道并启动首次接收 */
esp_err_t bsp_ir_rx_init(void);

/** 阻塞等待一帧 NEC 信号并解码；返回 address/command */
esp_err_t bsp_ir_rx_receive(uint16_t *address, uint16_t *command, uint32_t timeout_ms);

/** 红外接收示例 */
void bsp_ir_rx_demo(void);

#ifdef __cplusplus
}
#endif
