# I2C + XL9555 学习笔记

> 指南：第十九章 IIC_EXIO实验（`ref/DNESP32S3使用指南-IDF版_V1.7.pdf`）

## 硬件连接

- I2C0：SDA=IO41、SCL=IO42（`BSP_I2C0_*`），400kHz，内部上拉。
- XL9555：I2C 地址 `0x20`，16 路 IO 扩展（P00~P17）。
- XL9555 引脚功能：蜂鸣器 P03、按键 KEY0~KEY3（P17~P14）、LCD 背光/复位、摄像头 PWDN/RESET 等。

## 原理要点

- **I2C**：两线（SDA/SCL）同步串行总线，支持多设备（7 位地址）。
- 时序：起始位 → 器件地址 + 读写位 → 寄存器/数据 → 停止位。
- 每次写入前需发送寄存器地址（先写寄存器，再读写数据）。

## 关键 API（ESP-IDF `driver/i2c.h`）

| 函数 | 作用 |
|------|------|
| `i2c_param_config()` | 配置 I2C（主/从模式、引脚、上拉、时钟） |
| `i2c_driver_install()` | 安装驱动 |
| `i2c_cmd_link_create()` | 创建命令链 |
| `i2c_master_start/stop`、`i2c_master_write_byte`、`i2c_master_read_byte` | 构造时序 |
| `i2c_master_cmd_begin()` | 执行命令链 |

## 宏设计

- `BSP_I2C0_SCL_GPIO` / `BSP_I2C0_SDA_GPIO` / `BSP_I2C0_FREQ_HZ`。
- `BSP_XL9555_ADDR` / `BSP_XL9555_BEEP_IO`。

## 验证结果

- ✅ 编译通过，烧录成功
- ✅ I2C0 初始化正常（SDA=GPIO41 SCL=GPIO42 400kHz）
- ✅ XL9555 通信正常（`XL9555 init OK`，无 NACK 错误）
- ✅ 蜂鸣器周期性 ON/OFF
