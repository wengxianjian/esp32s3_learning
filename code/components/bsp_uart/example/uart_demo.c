/**
 * @file uart_demo.c
 * @brief UART 实验示例（对应指南第十三章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 初始化 UART0（115200-8-N-1，经板载 CH340 与 PC 通信）与 LED（接 IO1）；
 * 2) 每秒发送一行提示信息 "Hello UART, tick = N"，方便在串口助手里看到周期输出；
 * 3) 把 PC 发来的数据原样回显出去（回显实验），并可同时看到 LED 每周期翻转一次。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "UART" 这个 demo（对应宏 CONFIG_DEMO_UART），
 * app_main() 就会调用 bsp_uart_demo()。
 * 运行时请用串口助手打开板载 CH340 对应的端口（115200-8-N-1）。
 */

#include "bsp_uart.h"
#include "bsp_led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

/* 日志标签：本示例打印的日志会显示为 "uart_demo" */
static const char *TAG = "uart_demo";

/**
 * @brief UART 示例入口：周期发送提示 + 回显收到的数据 + LED 翻转
 *
 * 逻辑流程：
 *   ① 打印开始提示并初始化 UART0 与 LED
 *   ② 死循环：
 *        - 用 snprintf 拼出提示串（带自增计数 tick），通过 UART 发出；
 *        - 以 50ms 超时接收 PC 发来的数据，若收到则原样发回（回显）；
 *        - 翻转 LED 并延时 1s，进入下一轮
 */
void bsp_uart_demo(void)
{
    /* 提示开始，并提醒到 CH340 串口上观察 */
    ESP_LOGI(TAG, "UART demo start: periodic msg + echo (monitor CH340 port)");
    /* 初始化 UART0 与 LED；失败即断言复位 */
    ESP_ERROR_CHECK(bsp_uart_init());
    ESP_ERROR_CHECK(bsp_led_init());

    uint8_t rx_buf[128];   /* 接收缓冲区：一次最多收 128 字节 */
    uint32_t tick = 0;     /* 自增计数，仅用于在提示信息里体现"第几次发送" */

    while (1) {
        /* 定时发送提示信息 */
        char msg[64];
        /* snprintf 把内容按格式写入 msg 并返回将要写入的字符数 n（不含结尾 '\0'）；
         * sizeof(msg) 限定不越界；"\r\n" 是为了在串口助手里正确换行；
         * tick++ 先取当前值再自增，故每轮计数加 1 */
        int n = snprintf(msg, sizeof(msg), "Hello UART, tick = %lu\r\n",
                         (unsigned long)tick++);
        bsp_uart_send((const uint8_t *)msg, n);   /* 只发送 n 个有效字节（不含 '\0'） */

        /* 回显收到的数据：最多等 50ms，未收到则 len<=0 直接跳过 */
        int len = bsp_uart_recv(rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(50));
        if (len > 0) {
            bsp_uart_send(rx_buf, len);           /* 原样发回，实现"回显" */
        }

        bsp_led_toggle();                       /* 每轮翻转一次 LED，直观显示程序在运行 */
        vTaskDelay(pdMS_TO_TICKS(1000));        /* 1 秒一个周期，避免刷屏过快 */
    }
}
