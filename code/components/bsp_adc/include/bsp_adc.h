/**
 * @file bsp_adc.h
 * @brief ADC（模数转换）板级驱动接口 —— ADC1 通道 7（= GPIO8）
 *
 * ── 本实验做什么 ──────────────────────────────────────────────
 * 用 ESP32-S3 内置的 SAR ADC 采集 ADC_IN 排针上的模拟电压，把连续变化的
 * 电压转换成 0~4095 的数字量（原始值 raw），再近似换算成 mV 打印出来。
 * 对应正点原子《ESP32-S3 使用指南》第二十四章 ADC 实验。
 *
 * ── 硬件资源 ─────────────────────────────────────────────────
 * 采集引脚：ADC1_CHANNEL_7 = GPIO8（开发板 ADC_IN 排针）。
 * 板上自带一个电位器，用跳线帽把电位器接到 ADC_IN，旋转旋钮即可改变
 * 输入电压（约 0~3.3V），从而观察 raw / mV 的变化。
 *
 * ── 设计约定（本工程通用）────────────────────────────────────
 * 本文件只声明对外接口，**具体用哪个 ADC 单元、哪个通道、多大衰减、
 * 多少位分辨率，全部集中在 board.h 的 BSP_ADC_* 宏里定义**，代码只引用宏、
 * 不写死数字。这样想回顾"ADC 是怎么配的"，只要 grep BSP_ADC 就能一次命中
 * 宏定义处和所有使用点。
 *
 * ── 使用步骤 ─────────────────────────────────────────────────
 *   1) bsp_adc_init();          // 创建 ADC 单元并配置通道（必须最先调用）
 *   2) bsp_adc_read_raw();      // 读 12 位原始值 0~4095
 *   3) bsp_adc_read_mv();       // 读并换算成电压 mV
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 ADC：创建 ADC1 单元，并把目标通道配置为「11dB 衰减 + 12 位」
 *
 * 内部依次做两件事：
 *   1. adc_oneshot_new_unit()      —— 创建 ADC 单元（对应 BSP_ADC_UNIT）
 *   2. adc_oneshot_config_channel()—— 配置通道的衰减与位宽
 *
 * @return
 *      - ESP_OK   初始化成功
 *      - 其它     失败（创建单元或配置通道出错），具体错误码见串口日志
 *
 * @note 必须在 bsp_adc_read_raw() / bsp_adc_read_mv() 之前调用，且只需调用一次。
 */
esp_err_t bsp_adc_init(void);

/**
 * @brief 读取一次 ADC 原始值（未换算的数字量）
 *
 * @return 12 位原始值，范围 0 ~ 4095（0 对应约 0V，4095 对应满量程）
 *
 * @note 该值只是"数字量"，要得到电压需再换算，见 bsp_adc_read_mv()。
 */
int bsp_adc_read_raw(void);

/**
 * @brief 读取一次 ADC 并按线性关系换算成电压（单位 mV）
 *
 * 换算公式：mV = raw × 3100 / 4095
 *   - 3100：11dB 衰减下 ADC 的近似满量程电压（约 3.1V）
 *   - 4095：12 位 ADC 的最大原始值（2^12 - 1）
 *
 * @return 电压值，单位毫伏（mV），范围约 0 ~ 3100
 *
 * @note 这是**近似线性换算**：真实 ADC 存在非线性与零点偏移，工程上若要
 *       精确电压，应使用 ESP-IDF 的校准接口（adc_cali）；此处为教学演示，
 *       用线性近似即可观察电压随旋钮变化。
 */
uint32_t bsp_adc_read_mv(void);

/**
 * @brief ADC 示例入口：初始化后，周期性打印原始值与电压
 *
 * 该函数是 menuconfig 选择 DEMO_ADC 时由 app_main() 调用的演示主循环，
 * 会一直运行（内部 while(1)），每秒打印一次 raw 与 mV。
 */
void bsp_adc_demo(void);

#ifdef __cplusplus
}
#endif
