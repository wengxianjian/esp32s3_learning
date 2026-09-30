# 资源索引表

> 本表是「资源名 ↔ 宏前缀 ↔ board.h 定义 ↔ 组件目录 ↔ 指南章节 ↔ 学习状态」的映射总表。
> 与 `components/bsp_common/include/board.h` 配合：**宏只在 board.h 定义一处，此处只登记索引**。
> 回溯方法：`grep -rn "BSP_<宏前缀>" .`

## 已完成

| 资源 | 宏前缀 | 组件目录 | 指南章节 | 状态 |
|------|--------|----------|----------|------|
| LED | `BSP_LED0` | `components/bsp_led` | 第十章 LED实验 | ✅ 已完成 |

## 路线图（M0 ~ M4，逐资源推进）

### M0 环境跑通
| 资源 | 宏前缀 | 组件目录 | 指南章节 | 状态 |
|------|--------|----------|----------|------|
| LED | `BSP_LED0` | `components/bsp_led` | 第十章 | ✅ 已完成 |

### M1 基础外设
| 资源 | 宏前缀 | 组件目录 | 指南章节 | 状态 |
|------|--------|----------|----------|------|
| KEY 按键 | `BSP_KEY0` | `components/bsp_key` | 第十一章 | ✅ 已完成 |
| EXIT 外部中断 | `BSP_KEY0` | `components/bsp_key` | 第十二章 | ✅ 已完成 |
| UART 串口 | `BSP_UART0` | `components/bsp_uart` | 第十三章 | ✅ 已完成 |
| ESPTimer 软定时器 | `BSP_ESPTIMER` | `components/bsp_timer` | 第十四章 | ✅ 已完成 |
| GPTimer 硬定时器 | `BSP_GPTIMER` | `components/bsp_timer` | 第十五章 | ✅ 已完成 |
| 看门狗 | `BSP_WDT` | `components/bsp_timer` | 第十六章 | ✅ 已完成 |
| SW_PWM 软件 PWM | `BSP_PWM` | `components/bsp_pwm` | 第十七章 | ✅ 已完成 |
| HW_PWM 硬件 PWM(LEDC) | `BSP_PWM` | `components/bsp_pwm` | 第十八章 | ✅ 已完成 |
| ADC | `BSP_ADC` | `components/bsp_adc` | 第二十四章 | ✅ 已完成 |
| RTC | `BSP_RTC` | `components/bsp_rtc_rng` | 第二十三章 | ✅ 已完成 |
| RNG 随机数 | `BSP_RNG` | `components/bsp_rtc_rng` | 第三十一章 | ✅ 已完成 |
| 内部温度传感器 | `BSP_TSENS` | `components/bsp_rtc_rng` | 第二十八章 | ✅ 已完成 |

### M2 通信总线与 I2C 器件群
| 资源 | 宏前缀 | 组件目录 | 指南章节 | 状态 |
|------|--------|----------|----------|------|
| I2C 总线 | `BSP_I2C0` | `components/bsp_i2c` | 第十九~二十五章 | ✅ 已完成 |
| XL9555 IO 扩展 | `BSP_XL9555` | `components/bsp_i2c` | 第十九章 | ✅ 已完成 |
| I2C EEPROM | `BSP_EEPROM` | `components/bsp_i2c` | 第二十章 | ✅ 已完成 |
| I2C OLED | `BSP_OLED` | `components/bsp_i2c` | 第二十一章 | ✅ 已完成 |
| AP3216C 光强/接近 | `BSP_AP3216C` | `components/bsp_i2c` | 第二十五章 | ✅ 已完成 |
| QMA6100P 加速度 | `BSP_QMA6100P` | `components/bsp_i2c` | 第三十二章 | ✅ 已完成 |
| SPI 总线 / SPI LCD | `BSP_SPI2` | `components/bsp_spi` | 第二十二章 | ✅ 已完成 |
| 单总线 DS18B20 | `BSP_DS18B20` | `components/bsp_1wire` | 第二十九章 | ✅ 已完成 |
| 单总线 DHT11 | `BSP_DHT11` | `components/bsp_1wire` | 第三十章 | ✅ 已完成 |
| 红外接收 | `BSP_IR_RX` | `components/bsp_infrared` | 第二十六章 | ✅ 已完成 |
| 红外发送 | `BSP_IR_TX` | `components/bsp_infrared` | 第二十七章 | ✅ 已完成 |

### M3 显示与媒体
| 资源 | 宏前缀 | 组件目录 | 指南章节 | 状态 |
|------|--------|----------|----------|------|
| RGB/SPI LCD | `BSP_LCD` | `components/bsp_lcd` | 第二十二/三十四章 | ⬜ 待学习 |
| 触摸 | `BSP_TOUCH` | `components/bsp_lcd` | 第三十四章 | ⬜ 待学习 |
| 摄像头 | `BSP_CAMERA` | `components/bsp_camera` | 第三十五/三十六章 | ⬜ 待学习 |
| I2S 音频播放/录音 | `BSP_AUDIO` | `components/bsp_audio` | 第四十一/四十二章 | ⬜ 待学习 |
| SD 卡(SPI) | `BSP_SDCARD` | `components/bsp_sdcard` | 第三十七章 | ⬜ 待学习 |
| SPIFFS 文件系统 | `BSP_SPIFFS` | `components/bsp_sdcard` | 第三十八章 | ⬜ 待学习 |
| 汉字/图片/视频显示 | `BSP_LCD` | `components/bsp_lcd` | 第三十九~四十三章 | ⬜ 待学习 |

### M4 USB
| 资源 | 宏前缀 | 组件目录 | 指南章节 | 状态 |
|------|--------|----------|----------|------|
| Flash 模拟 U 盘 | `BSP_USB_MSC` | `components/bsp_usb` | 第四十五章 | ⬜ 待学习 |
| SD 卡模拟 U 盘 | `BSP_USB_MSC` | `components/bsp_usb` | 第四十六章 | ⬜ 待学习 |
