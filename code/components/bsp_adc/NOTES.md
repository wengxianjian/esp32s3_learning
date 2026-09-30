# ADC 学习笔记

> 指南：第二十四章 ADC实验（`ref/DNESP32S3使用指南-IDF版_V1.7.pdf`）

## 硬件连接

- ADC1 通道 7 = **GPIO8**（`BSP_ADC_CHANNEL`），对应开发板 `ADC_IN` 排针。
- 开发板有电位器，可通过跳线帽把电位器接到 ADC_IN，调节 0~3.3V 电压。

## 原理要点

- **ADC（模数转换器）**：把连续的模拟信号（电压）转换为离散的数字信号。
- 转换过程：采样 → 保持 → 量化 → 编码。
- ESP32-S3 集成 2 个 SAR ADC（ADC1、ADC2），支持多个模拟通道。
- **衰减（attenuation）**：决定量程。`ADC_ATTEN_DB_11` 量程约 0~3.1V。
- 分辨率 12 位，原始值范围 0~4095。

## 关键 API（ESP-IDF `esp_adc/adc_oneshot.h`，新版）

| 函数 | 作用 |
|------|------|
| `adc_oneshot_new_unit()` | 创建 ADC 单元 |
| `adc_oneshot_config_channel()` | 配置通道（衰减/位宽） |
| `adc_oneshot_read()` | 读取原始值 |

> 旧版 `adc1_get_raw()` 已废弃，本工程用新版 oneshot API。

## 宏设计

- `BSP_ADC_UNIT` / `BSP_ADC_CHANNEL` / `BSP_ADC_ATTEN` / `BSP_ADC_BITWIDTH`。

## 验证结果

- ✅ 编译通过，烧录成功
- ✅ ADC1 通道 7 初始化正常
- ✅ 周期性打印 raw/电压（约 150~180mV，未接电位器时为浮空噪声；接电位器后随旋钮变化）
