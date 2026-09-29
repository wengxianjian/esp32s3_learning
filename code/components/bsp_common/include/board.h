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
 * 后续资源宏在此按「每学习一个资源就补充一段」的规则追加。
 * 示例（待后续里程碑填充）：
 *   KEY     -> BSP_KEY0_GPIO
 *   UART    -> BSP_UART0_TX_GPIO / BSP_UART0_RX_GPIO
 *   I2C     -> BSP_I2C0_SCL_GPIO / BSP_I2C0_SDA_GPIO / BSP_I2C0_FREQ_HZ
 *   SPI     -> BSP_SPI2_SCK_GPIO / ...
 * ============================================================ */

#ifdef __cplusplus
}
#endif
