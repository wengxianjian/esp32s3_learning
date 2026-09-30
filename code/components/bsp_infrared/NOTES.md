# 红外收发（RMT + NEC）学习笔记

## 硬件
- 红外接收头 REMOTE_IN=IO2（与 SD 卡 TF_CS 复用，不能同时用）。
- 红外发送 REMOTE_OUT=IO8（与 ADC_IN/LCD_G5 复用）。
- 38kHz 载波，NEC 协议。

## 原理要点
- RMT 外设专为红外/遥控设计，可收发精确时序脉冲。
- 发送：RMT TX + 38kHz 载波 + NEC 编码器（前导码 + 16bit 地址 + 16bit 命令 + 结束码）。
- 接收：RMT RX（1MHz 分辨率）+ 符号解析 → NEC 解码。
- NEC 时序：前导 9ms+4.5ms；bit0=560us+560us；bit1=560us+1690us。
- 解码容差 ~200us。

## 宏设计
- `BSP_IR_RX_GPIO`、`BSP_IR_TX_GPIO`、`BSP_IR_RESOLUTION_HZ`、`BSP_IR_NEC_DECODE_MARGIN`。

## 验证结果

- ✅ 发送：`IR TX init OK (GPIO8, 38kHz)`，每 2s 发一帧 NEC（0x0440/0x3003）
- ✅ 接收：`IR RX init OK (GPIO2)`，等待红外信号（解码需红外遥控器触发）
