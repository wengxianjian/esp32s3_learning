/**
 * @file bsp_uart.h
 * @brief UART0 板级驱动接口（经板载 CH340 与 PC 通信）
 *
 * 引脚与波特率由 board.h 中的 BSP_UART0_* 宏决定，本驱动不写死任何参数。
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 初始化 UART0（8N1，无流控） */
esp_err_t bsp_uart_init(void);

/** 发送数据，返回实际发送字节数（<0 表示失败） */
int bsp_uart_send(const uint8_t *data, size_t len);

/** 接收数据，返回实际接收字节数（超时返回 0，<0 表示失败） */
int bsp_uart_recv(uint8_t *buf, size_t len, TickType_t timeout);

/** UART 示例：回显 + 定时发提示 + LED 闪烁 */
void bsp_uart_demo(void);

#ifdef __cplusplus
}
#endif
