/**
 * @file bsp_uart.c
 * @brief UART0 板级驱动实现（TX=GPIO43 / RX=GPIO44，经板载 CH340 与 PC 通信）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 封装 ESP-IDF 的 UART 驱动，对外提供三个函数：
 *   bsp_uart_init()  —— 配置串口参数、绑定引脚并安装收发驱动
 *   bsp_uart_send()  —— 发送一段数据，返回实际发送的字节数
 *   bsp_uart_recv()  —— 在给定超时时间内接收数据，返回实际收到的字节数
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * 使用 UART0（BSP_UART0_NUM），参数为 115200-8-N-1：波特率 115200、8 位数据位、
 * 1 位停止位、无校验位，且不使用硬件流控，时钟源采用默认值（UART_SCLK_DEFAULT）。
 * 数据流：
 *   发送：任务调用 bsp_uart_send() → uart_write_bytes() 把数据写入 UART0 发送
 *         FIFO → 经 TX(IO43) 送出 → 板载 USB 转串口芯片 CH340 → PC 串口监视器；
 *   接收：PC 发来数据 → RX(IO44) → UART0 接收 FIFO → 驱动内部接收环形缓冲区
 *         → bsp_uart_recv() 从缓冲区取走并返回字节数。
 * 安装驱动时分配了内部缓冲（rx 2048、tx 2048 字节），收发均由驱动在后台用中断
 * 搬运，用户任务只需调用阻塞式读写 API 操作缓冲区，无需关心中断细节。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 端口号、TX/RX 引脚、波特率均来自 board.h 的 BSP_UART0_* 宏，本文件不写死数字。
 * 指南章节：第十三章 UART 实验。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * ① UART0 默认也是 IDF 日志（printf）的输出口，用它收发自定义数据时，日志会与
 *    自定义数据混在同一串口，属正常现象；
 * ② uart_set_pin() 的 RTS/CTS 传 UART_PIN_NO_CHANGE，因为未启用硬件流控；
 * ③ uart_driver_install() 的队列长度传 0，表示不注册事件队列（本驱动用阻塞读，
 *    而非事件驱动）；
 * ④ 日志标签统一为 "bsp_uart"。
 */

#include "bsp_uart.h"
#include "board.h"
#include "driver/uart.h"
#include "esp_log.h"

/* 日志标签：打印串口相关日志时统一显示为 "bsp_uart" */
static const char *TAG = "bsp_uart";

/**
 * @brief 初始化 UART0：115200-8-N-1，绑定 TX/RX 引脚并安装收发驱动
 *
 * 逻辑流程：
 *   ① uart_param_config() 设置波特率/数据位/校验位/停止位/流控/时钟源
 *   ② uart_set_pin() 绑定 TX/RX 引脚（RTS/CTS 保持不变）
 *   ③ uart_driver_install() 安装驱动并分配收发内部缓冲区
 *   ④ 打印实际参数，便于对照硬件与 PC 端设置
 */
esp_err_t bsp_uart_init(void)
{
    uart_config_t cfg = {
        .baud_rate = BSP_UART0_BAUD_RATE,       /* 波特率：来自 board.h，本板 115200 */
        .data_bits = UART_DATA_8_BITS,          /* 数据位：8 位 */
        .parity = UART_PARITY_DISABLE,          /* 校验位：无校验 */
        .stop_bits = UART_STOP_BITS_1,          /* 停止位：1 位 */
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,  /* 流控：不使用硬件流控（RTS/CTS） */
        .source_clk = UART_SCLK_DEFAULT,        /* 时钟源：用 IDF 默认时钟 */
    };

    /* 应用串口参数配置 */
    esp_err_t ret = uart_param_config(BSP_UART0_NUM, &cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 绑定引脚：TX/RX 取自 board.h；RTS/CTS 传 UART_PIN_NO_CHANGE 表示不改动
     * （未启用流控，不需要这两个信号） */
    ret = uart_set_pin(BSP_UART0_NUM, BSP_UART0_TX_GPIO, BSP_UART0_RX_GPIO,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 安装驱动：接收缓冲 2048 字节、发送缓冲 2048 字节、不使用事件队列
     * （queue_size=0，事件队列句柄传 NULL）、中断分配标志为 0（用默认值） */
    ret = uart_driver_install(BSP_UART0_NUM, 2048, 2048, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 打印实际生效的参数，便于与 PC 串口助手设置核对 */
    ESP_LOGI(TAG, "UART0 init: %d baud, TX=GPIO%d RX=GPIO%d",
             BSP_UART0_BAUD_RATE, BSP_UART0_TX_GPIO, BSP_UART0_RX_GPIO);
    return ESP_OK;
}

/**
 * @brief 通过 UART0 发送一段数据
 *
 * 直接调用 uart_write_bytes()：数据先进入发送缓冲区，由驱动在后台搬运到硬件。
 *
 * @param data 待发送数据首地址
 * @param len  待发送字节数
 * @return 实际写入发送缓冲区的字节数；<0 表示失败
 */
int bsp_uart_send(const uint8_t *data, size_t len)
{
    return uart_write_bytes(BSP_UART0_NUM, data, len);   /* 阻塞式写入发送缓冲区 */
}

/**
 * @brief 从 UART0 接收一段数据
 *
 * 直接调用 uart_read_bytes()：从接收缓冲区取数据；若在 timeout 内没有收到任何
 * 数据则超时返回 0。
 *
 * @param buf     接收缓冲区首地址
 * @param len     期望接收的最大字节数
 * @param timeout 超时时间（FreeRTOS tick，可用 pdMS_TO_TICKS() 换算）
 * @return 实际接收到的字节数；超时返回 0；<0 表示失败
 */
int bsp_uart_recv(uint8_t *buf, size_t len, TickType_t timeout)
{
    return uart_read_bytes(BSP_UART0_NUM, buf, len, timeout);   /* 带超时的阻塞式读取 */
}
