# SPI 总线 + SPI LCD（ILI9341）学习笔记

## 硬件
- ILI9341 控制器，2.4 寸 320x240，SPI 4 线接口。
- SPI2：MOSI=IO11、SCK=IO12、MISO=IO13。
- CS=IO21、DC(WR)=IO40；RST、背光走 XL9555（P12=0x0400、P13=0x0800）。

## 原理要点
- SPI 4 线（CS/SCK/MOSI/DC）：DC=0 传命令，DC=1 传数据，CS 由 SPI 外设自动管理。
- RGB565 16bit 色深，一个像素 2 字节（R5 G6 B5）。
- ILI9341 关键命令：`0x2A` 列地址、`0x2B` 页地址、`0x2C` 写内存、`0x36` 内存访问控制（方向）、`0x3A` 像素格式、`0x29` 开显示。
- 初始化需完整命令序列（电源控制、gamma、帧率等，参考 Adafruit/规格书）。

## 宏设计
- `BSP_SPI2_MOSI/SCLK/MISO_GPIO`、`BSP_LCD_CS_GPIO`、`BSP_LCD_DC_GPIO`、`BSP_LCD_RST_IO`、`BSP_LCD_PWR_IO`、`BSP_LCD_WIDTH/HEIGHT`。

## 验证结果

- ✅ `ILI9341 init OK (320x240)`，屏幕循环切换 8 种背景色（红/绿/蓝/黄/青/品红/白/黑）

## 踩坑记录

- `bsp_xl9555_init()` 内部未先初始化 I2C0，直接调用 `bsp_i2c_write_reg` 会返回 `ESP_ERR_INVALID_STATE`（驱动未安装）。
  修复：`bsp_i2c_init()` 改为幂等（静态标志位），`bsp_xl9555_init()` 先调用 `bsp_i2c_init()`。

> 说明：文字/汉字/图片显示在 M3（RGB LCD 章节，第三十九~四十三章）统一实现，此处先验证 SPI + ILI9341 的彩色填充。
