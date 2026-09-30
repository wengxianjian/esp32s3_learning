/**
 * @file board.h
 * @brief DNESP32S3 开发板资源宏定义字典（全工程唯一的资源定义处）
 *
 * 命名规范：BSP_<资源名>_<信号/属性>
 *   - 资源名：led / key / uart / i2c / spi / lcd / camera / audio / ...
 *   - 信号/属性：GPIO / ACTIVE_LEVEL / SCL / SDA / FREQ_HZ / ...
 *
 * 使用规则（重要）：
 *   1. 所有组件代码只引用这里的宏，绝不写死引脚号；
 *   2. 通过 `grep -rn "BSP_<资源名>"` 即可回溯该资源在工程中的所有用法；
 *   3. 与 RESOURCES.md 索引表配合，形成「资源名 <-> 宏前缀 <-> 组件目录 <-> 指南章节」互查。
 *
 * 引脚来源：ref/DNESP32-S3 IO引脚分配表.xlsx + ref/ATK_DNESP32S3_V1.2.pdf 原理图
 */
#pragma once

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * LED（用户 LED）
 * 指南：第十章 LED实验
 * 引脚：IO1，灌电流接法（低电平点亮）
 * ============================================================ */
#define BSP_LED0_GPIO            GPIO_NUM_1
#define BSP_LED0_ACTIVE_LEVEL    0       /* 0=低电平点亮，1=高电平点亮 */

/* ============================================================
 * KEY（BOOT 独立按键）
 * 指南：第十一章 KEY实验
 * 引脚：IO0，低电平有效（按下=0），内部上拉
 * ============================================================ */
#define BSP_KEY0_GPIO            GPIO_NUM_0
#define BSP_KEY0_ACTIVE_LEVEL    0       /* 0=按下为低电平 */
#define BSP_KEY0_INTR_TYPE       GPIO_INTR_NEGEDGE   /* 按键外部中断：下降沿触发 */

/* ============================================================
 * UART（串口）
 * 指南：第十三章 UART实验
 * 使用 UART0，经板载 USB 转串口芯片（CH340）与 PC 通信
 * ============================================================ */
#define BSP_UART0_NUM            UART_NUM_0
#define BSP_UART0_TX_GPIO        GPIO_NUM_43
#define BSP_UART0_RX_GPIO        GPIO_NUM_44
#define BSP_UART0_BAUD_RATE      115200

/* ============================================================
 * ESPTimer 软件定时器
 * 指南：第十四章 ESPTIMER实验
 * ============================================================ */
#define BSP_ESPTIMER_PERIOD_US    500000   /* 定时周期 500ms */

/* ============================================================
 * GPTimer 硬件定时器
 * 指南：第十五章 GPTIMER实验
 * ============================================================ */
#define BSP_GPTIMER_PERIOD_US     500000   /* 定时周期 500ms */

/* ============================================================
 * Watchdog 任务看门狗（TWDT）
 * 指南：第十六章 WATCH_DOG实验
 * ============================================================ */
#define BSP_WDT_TIMEOUT_MS        3000     /* 看门狗超时时间 3s */

/* ============================================================
 * PWM（LEDC，软/硬件 PWM）
 * 指南：第十七章 SW_PWM / 第十八章 HW_PWM
 * ============================================================ */
#define BSP_PWM_LEDC_TIMER        LEDC_TIMER_0
#define BSP_PWM_LEDC_CHANNEL      LEDC_CHANNEL_0
#define BSP_PWM_FREQ_HZ           1000     /* PWM 频率 1kHz */
#define BSP_PWM_DUTY_RES          LEDC_TIMER_10_BIT
#define BSP_PWM_DUTY_MAX          1023     /* 10 位分辨率对应的最大占空比 */

/* ============================================================
 * ADC（模数转换）
 * 指南：第二十四章 ADC实验
 * 通道：ADC1_CHANNEL_7 = GPIO8（ADC_IN 排针）
 * ============================================================ */
#define BSP_ADC_UNIT              ADC_UNIT_1
#define BSP_ADC_CHANNEL           ADC_CHANNEL_7
#define BSP_ADC_ATTEN             ADC_ATTEN_DB_11    /* 量程约 0~3.1V */
#define BSP_ADC_BITWIDTH          ADC_BITWIDTH_12

/* ============================================================
 * 后续资源宏在此按「每学习一个资源就补充一段」的规则追加。
 * ============================================================ */

#ifdef __cplusplus
}
#endif
