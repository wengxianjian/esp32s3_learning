# PWM 学习笔记（LEDC 软/硬件 PWM）

> 指南：第十七章 SW_PWM / 第十八章 HW_PWM（`ref/DNESP32S3使用指南-IDF版_V1.7.pdf`）

## 硬件连接

- LED 连接 IO1（`BSP_LED0_GPIO`），LEDC 通道输出到该引脚。

## 原理要点

- **PWM（脉冲宽度调制）**：把模拟信号（如亮度）转换为脉冲信号。
  - 频率：1s 内 PWM 周期数（Hz）。
  - 周期：频率的倒数。
  - **占空比**：一个周期内高电平时间占比（0~100%）。
- 高频 PWM 驱动 LED 时，人眼无法分辨闪烁，占空比越高 LED 越亮（对共阳/灌电流接法则相反）。
- ESP32-S3 的 **LEDC（LED 控制器）** 提供多路硬件 PWM，无需 CPU 干预。

## 软 PWM vs 硬 PWM

| 方式 | 实现 | 特点 |
|------|------|------|
| 软 PWM（第十七章） | 循环里调用 `ledc_set_duty()` 手动改占空比 | CPU 参与 |
| 硬 PWM（第十八章） | LEDC 硬件渐变函数 `ledc_set_fade_with_time()` | 硬件自动渐变，CPU 几乎不参与 |

## 关键 API（ESP-IDF `driver/ledc.h`）

| 函数 | 作用 |
|------|------|
| `ledc_timer_config()` | 配置定时器（频率/分辨率） |
| `ledc_channel_config()` | 配置通道（GPIO/定时器/初始占空比） |
| `ledc_set_duty()` + `ledc_update_duty()` | 设置并更新占空比 |
| `ledc_fade_func_install()` | 安装渐变功能 |
| `ledc_set_fade_with_time()` + `ledc_fade_start()` | 硬件渐变 |

## 宏设计

- `BSP_PWM_LEDC_TIMER` / `BSP_PWM_LEDC_CHANNEL` / `BSP_PWM_FREQ_HZ` / `BSP_PWM_DUTY_MAX`。

## 验证结果

- ✅ 编译通过，烧录成功
- ✅ 软 PWM：`LEDC PWM init: 1000 Hz, GPIO 1`，LED 呼吸（循环改占空比）
- ✅ 硬 PWM：`LEDC PWM init: 1000 Hz, GPIO 1`，LED 呼吸（硬件渐变）
