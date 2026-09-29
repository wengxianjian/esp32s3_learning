# LED 学习笔记

> 指南：第十章 LED实验（`ref/DNESP32S3使用指南-IDF版_V1.7.pdf`）

## 硬件连接

- 用户 LED 连接在 **IO1**（`board.h` 中 `BSP_LED0_GPIO`）。
- 采用**灌电流接法**（sink current）：LED 阳极接 3.3V，阴极经限流电阻接 GPIO。
- 因此 **GPIO 输出低电平 → LED 点亮，高电平 → 熄灭**（`BSP_LED0_ACTIVE_LEVEL = 0`）。

## 原理要点

- **灌电流（拉电流）**：电流从外部电源流入 MCU 引脚，由 GPIO 内部开关管吸收。
  - 优点：避免由 MCU 直接"推"出大电流，减轻 MCU 负载，稳定性好。
  - 对比**拉电流**：MCU 引脚输出高电平直接驱动 LED，驱动能力弱。
- ESP32-S3 的 GPIO 输出能力有限（约 20mA 级），驱动大电流负载应使用灌电流或外接三极管/MOS。

## 关键 API（ESP-IDF）

| 函数 | 作用 |
|------|------|
| `gpio_config()` | 配置引脚（方向、上下拉、中断等），参数为 `gpio_config_t` |
| `gpio_set_level()` | 设置单个引脚输出电平 |
| `gpio_get_level()` | 读取引脚当前电平 |

## 宏设计

- `BSP_LED0_GPIO`：LED 引脚（定义于 `board.h`）。
- `BSP_LED0_ACTIVE_LEVEL`：有效电平（0=低电平点亮）。
- 驱动通过 `BSP_LED0_ACTIVE_LEVEL` 换算，因此**即使换引脚或换有效电平，只改 board.h 即可**。

## 踩坑记录

- 引脚必须先在 `board.h` 定义，驱动里不写死 `GPIO_NUM_1`，保证 grep 宏可回溯。
- `bsp_led_set()` 里要做有效电平换算，避免调用方误以为"true=高电平"。

## 验证结果

- ✅ 编译通过，烧录成功（bootloader + 分区表 + app 均写入并校验通过）
- ✅ LED 按 500ms 间隔正常闪烁（低电平点亮，IO1 正确）
- ✅ 启动日志确认：`bsp_led: LED initialized on GPIO 1`
- 板载 Flash 规格：16MB、DIO 模式（已配置到 `sdkconfig.defaults`）

## 烧录方式（免手动按键）

本开发板有**两路 USB**：

| 端口 | 设备 | 特点 |
|------|------|------|
| USB_UART | `/dev/cu.usbserial-*`（CH340） | DTR/RTS 自动下载电路在 macOS 上不可靠，需手动 BOOT+RESET |
| USB | `/dev/cu.usbmodem*`（原生 USB-Serial-JTAG） | **自带可靠自动下载/复位，全程免按键，且速度更快** |

推荐统一走 usbmodem（工程已配置 `CONFIG_ESPTOOLPY_BEFORE_USB_RESET` 与控制台走 USB-Serial-JTAG）：

```bash
idf.py -p /dev/cu.usbmodem* build flash monitor
```
