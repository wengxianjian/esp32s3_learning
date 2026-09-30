# RTC / RNG / 内部温度传感器 学习笔记

> 指南：第二十三章 RTC / 第三十一章 RNG / 第二十八章 内部温度传感器

## RTC（实时时钟）

- ESP32-S3 没有专门 RTC 外设 API，通过 C 标准库 `settimeofday()` 设置系统时间、`localtime()` 读取。
- `mktime()` 把 `struct tm` 转为 `time_t`（秒）。
- 系统时间由 RTC 硬件保持。

| 函数 | 作用 |
|------|------|
| `settimeofday()` | 设置系统时间 |
| `time()` / `localtime_r()` | 获取当前时间 |

## RNG（随机数）

- `esp_random()` 返回 32 位硬件随机数，无需初始化。
- `rng_range(min, max)`：`esp_random() % (max - min + 1) + min` 得到区间随机数。

## 内部温度传感器（TSENS）

- 使用 `driver/temperature_sensor.h`。
- 需配置测量范围 `range_min`/`range_max`（本工程 -10~80℃）。
- 读取前 `temperature_sensor_enable()`，读完 `disable()` 省电。

| 函数 | 作用 |
|------|------|
| `temperature_sensor_install()` | 安装（配置量程/时钟源） |
| `temperature_sensor_enable()` | 使能 |
| `temperature_sensor_get_celsius()` | 读取摄氏温度 |
| `temperature_sensor_disable()` | 失能 |

## 宏设计

- `BSP_TSENS_RANGE_MIN` / `BSP_TSENS_RANGE_MAX`。
- RTC / RNG 无引脚宏（内部资源）。

## 验证结果

- ✅ RTC：时间从 `2024-01-01 00:00:00` 每秒递增
- ✅ RNG：每秒打印不同的随机数 + 区间随机数
- ✅ TSENS：芯片温度 `33.50 C`，稳定
