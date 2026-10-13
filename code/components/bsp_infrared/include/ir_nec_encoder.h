/**
 * @file ir_nec_encoder.h
 * @brief 红外 NEC 协议 RMT 编码器接口（改编自 ESP-IDF ir_nec_transceiver 示例）
 *
 * ── 本文件做什么 ──────────────────────────────────────────────
 * 声明一个可挂到 RMT TX 通道上的自定义编码器：把"16 位地址 + 16 位命令"
 * 编码成 NEC 标准红外波形。实现见同目录 ir_nec_encoder.c。
 *
 * ── 工作原理 ─────────────────────────────────────────────────
 * 输入数据为 ir_nec_scan_code_t；编码器据此产生引导码、32 位数据（低位在先）
 * 与结束码的电平序列，再由 RMT 通道的 38kHz 载波调制成红外脉冲发出。
 *
 * ── 使用步骤 ─────────────────────────────────────────────────
 *   1) rmt_new_ir_nec_encoder()：创建编码器（需先有 RMT TX 通道）；
 *   2) 通过 rmt_transmit() 把 ir_nec_scan_code_t 交给该编码器发送。
 *
 * ── 注意事项 ─────────────────────────────────────────────────
 * 本文件改编自 ESP-IDF ir_nec_transceiver 示例（Apache-2.0）。
 */

/*
 * 来自 ESP-IDF ir_nec_transceiver 示例（Apache-2.0）
 */
#pragma once

#include <stdint.h>
#include "driver/rmt_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

/** IR NEC 扫描码：一帧要发送的内容（16 位地址 + 16 位命令） */
typedef struct {
    uint16_t address;
    uint16_t command;
} ir_nec_scan_code_t;

/** 编码器配置：只需给出 RMT 的分辨率（Hz），用于把微秒换算成 RMT 计数 */
typedef struct {
    uint32_t resolution;
} ir_nec_encoder_config_t;

/**
 * @brief 创建 NEC 编码器
 *
 * @param config      编码器配置（含 RMT 分辨率），不可为空
 * @param ret_encoder 输出：创建好的编码器句柄
 * @return ESP_OK 成功；ESP_ERR_INVALID_ARG 参数为空；ESP_ERR_NO_MEM 内存不足
 */
esp_err_t rmt_new_ir_nec_encoder(const ir_nec_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder);

#ifdef __cplusplus
}
#endif
