# UART 学习笔记

> 指南：第十三章 UART实验（`ref/DNESP32S3使用指南-IDF版_V1.7.pdf`）

## 硬件连接

- 使用 **UART0**：TX=GPIO43、RX=GPIO44（`board.h` 中 `BSP_UART0_*`）。
- UART0 经板载 USB 转串口芯片（CH340）与 PC 通信（跳线帽连接）。
- 本工程日志控制台已切到 USB-Serial-JTAG（`usbmodem`），因此 UART0 可专门用于实验，其输出走 CH340 的 `usbserial` 端口。

## 原理要点

- **串口通信协议**：数据包由起始位、数据位、校验位、停止位组成；双方格式必须一致。
- **波特率**：每秒传输的码元个数，通信双方必须一致（常用 115200）。
- **数据格式**：常见 8N1（8 数据位、无校验、1 停止位）。
- ESP32-S3 有 **3 个 UART 控制器**（UART0/1/2）。
- UART0 由 APB 时钟驱动，需配置 `source_clk`（本工程用 `UART_SCLK_DEFAULT`）。

## 关键 API（ESP-IDF）

| 函数 | 作用 |
|------|------|
| `uart_param_config()` | 配置波特率/数据位/校验/停止位/流控/时钟源 |
| `uart_set_pin()` | 指定 TX/RX 引脚 |
| `uart_driver_install()` | 安装驱动并分配收发缓冲区 |
| `uart_write_bytes()` | 发送数据 |
| `uart_read_bytes()` | 读取数据（带超时） |

## 宏设计

- `BSP_UART0_NUM` / `BSP_UART0_TX_GPIO` / `BSP_UART0_RX_GPIO` / `BSP_UART0_BAUD_RATE`。
- 驱动只引用宏，不写死端口号和引脚。

## 验证结果

- ✅ 编译通过，烧录成功
- ✅ UART0 初始化正常：`115200 baud, TX=GPIO43 RX=GPIO44`
- ✅ 定时发送：CH340 端口（`usbserial`）收到 `Hello UART, tick = N`（每秒递增）
- ✅ 回显：向 CH340 端口写入 `ECHO_TEST_123`，被原样返回
- ✅ LED 每秒闪烁
