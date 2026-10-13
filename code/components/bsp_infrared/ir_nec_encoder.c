/**
 * @file ir_nec_encoder.c
 * @brief 红外 NEC 协议 RMT 编码器实现（改编自 ESP-IDF ir_nec_transceiver 示例）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 实现一个可挂到 RMT TX 通道上的自定义编码器：把 ir_nec_scan_code_t
 * （16 位地址 + 16 位命令）编码成 NEC 标准波形。对外提供：
 *   rmt_new_ir_nec_encoder() —— 创建并初始化编码器实例
 * 另有 static 回调：rmt_encode_ir_nec（核心编码）、rmt_del_ir_nec_encoder
 * （删除/回收）、rmt_ir_nec_encoder_reset（复位状态机）。
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * 编码器采用"组合 + 状态机"设计：内部持有一个 copy_encoder（原样拷贝符号）
 * 和一个 bytes_encoder（按位把 16bit 数据编码成 0/1 波形），并用 state
 * 记录进度，通过多次调用把一帧分四段依次产出（RMT 内存满时会分批继续）：
 *   ① state=0：拷贝 nec_leading_symbol —— 引导码 9ms 脉冲 + 4.5ms 空闲
 *   ② state=1：用 bytes_encoder 编码 address（16bit，低位在先）
 *   ③ state=2：用 bytes_encoder 编码 command（16bit）
 *   ④ state=3：拷贝 nec_ending_symbol —— 结束脉冲（560us 脉冲 + 超长空闲）
 * 每段产出后若返回 RMT_ENCODING_MEM_FULL，说明 RMT 内存暂时写满，
 * 需先返回、等下次调用从当前 state 继续（switch 有意 fall-through）。
 * 波形中的高电平段（level0=1）会被通道的 38kHz 载波调制为红外脉冲。
 * bytes_encoder 的位定义：bit0 = 560us 脉冲 + 560us 空闲；
 * bit1 = 560us 脉冲 + 1690us 空闲，与 NEC 标准一致。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 无板级引脚宏依赖；时序时长按 config->resolution（RMT 分辨率）把"微秒"
 * 换算成 RMT 计数值：duration = us * resolution / 1000000。
 * 指南章节：第二十七章 红外发送实验（本文件为发送链路的编码器部件）。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * - 本文件改编自 ESP-IDF ir_nec_transceiver 示例（Apache-2.0）。
 * - 错误处理统一用 ESP_GOTO_ON_FALSE / ESP_GOTO_ON_ERROR 跳到 err 标签，
 *   保证中途失败也能正确释放已分配资源，避免内存泄漏。
 * - state 的 fall-through 是刻意为之（注释 "fall-through"），不是笔误；
 *   每次进入函数都从上次未完成的 state 继续，不能在 case 间加 break。
 */

/*
 * 来自 ESP-IDF ir_nec_transceiver 示例（Apache-2.0）
 */
#include "esp_check.h"
#include "ir_nec_encoder.h"

/* 日志标签：编码器相关日志显示为 "nec_encoder" */
static const char *TAG = "nec_encoder";

/* NEC 编码器实例结构体：
 *   base                —— 编码器基类（RMT 通过它调用 encode/del/reset）
 *   copy_encoder        —— 原样拷贝符号的子编码器（发引导码/结束码用）
 *   bytes_encoder       —— 按位编码 0/1 的子编码器（发地址/命令用）
 *   nec_leading_symbol  —— 引导码符号（9ms 脉冲 + 4.5ms 空闲）
 *   nec_ending_symbol   —— 结束码符号（560us 脉冲 + 长空闲）
 *   state               —— 状态机进度（0~3，见 rmt_encode_ir_nec）
 */
typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *copy_encoder;
    rmt_encoder_t *bytes_encoder;
    rmt_symbol_word_t nec_leading_symbol;
    rmt_symbol_word_t nec_ending_symbol;
    int state;
} rmt_ir_nec_encoder_t;

/**
 * @brief 编码器核心：把一帧 NEC 数据编码成 RMT 符号序列
 *
 * @param encoder     编码器基类指针（用 __containerof 反推回实例）
 * @param channel     所属 RMT 通道
 * @param primary_data 输入数据（此处为 ir_nec_scan_code_t*）
 * @param data_size   输入数据大小
 * @param ret_state   输出：本次编码状态
 * @return 本次编码产出的符号个数
 *
 * 逻辑流程：
 *   ① 由基类指针反推回 rmt_ir_nec_encoder_t 实例，取出各子编码器
 *   ② 依 nec_encoder->state 决定从哪一段继续，逐段编码：
 *      state0 引导码 → state1 地址 → state2 命令 → state3 结束码
 *   ③ 每段若完成则推进 state；若内存满则带 RMT_ENCODING_MEM_FULL 提前返回
 *   ④ 四段全部完成后把 state 复位为 RESET 并标记 RMT_ENCODING_COMPLETE
 */
static size_t rmt_encode_ir_nec(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
    rmt_ir_nec_encoder_t *nec_encoder = __containerof(encoder, rmt_ir_nec_encoder_t, base);
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;
    ir_nec_scan_code_t *scan_code = (ir_nec_scan_code_t *)primary_data;
    rmt_encoder_handle_t copy_encoder = nec_encoder->copy_encoder;
    rmt_encoder_handle_t bytes_encoder = nec_encoder->bytes_encoder;
    switch (nec_encoder->state) {
    case 0:
        /* ① 发引导码：原样拷贝 nec_leading_symbol */
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &nec_encoder->nec_leading_symbol,
                                                sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) nec_encoder->state = 1;
        if (session_state & RMT_ENCODING_MEM_FULL) { state |= RMT_ENCODING_MEM_FULL; goto out; }
    // fall-through
    case 1:
        /* ② 编码 16 位地址（bytes_encoder 按位展开） */
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->address, sizeof(uint16_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) nec_encoder->state = 2;
        if (session_state & RMT_ENCODING_MEM_FULL) { state |= RMT_ENCODING_MEM_FULL; goto out; }
    // fall-through
    case 2:
        /* ③ 编码 16 位命令 */
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->command, sizeof(uint16_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) nec_encoder->state = 3;
        if (session_state & RMT_ENCODING_MEM_FULL) { state |= RMT_ENCODING_MEM_FULL; goto out; }
    // fall-through
    case 3:
        /* ④ 发结束码：原样拷贝 nec_ending_symbol；完成后复位状态机 */
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &nec_encoder->nec_ending_symbol,
                                                sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            nec_encoder->state = RMT_ENCODING_RESET;
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) state |= RMT_ENCODING_MEM_FULL;
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

/**
 * @brief 删除编码器并释放其占用的资源
 *
 * @param encoder 编码器基类指针
 * @return ESP_OK
 *
 * 逻辑流程：
 *   ① 反推回实例，删除内部的 copy/bytes 子编码器
 *   ② 释放实例自身内存
 */
static esp_err_t rmt_del_ir_nec_encoder(rmt_encoder_t *encoder)
{
    rmt_ir_nec_encoder_t *nec_encoder = __containerof(encoder, rmt_ir_nec_encoder_t, base);
    rmt_del_encoder(nec_encoder->copy_encoder);
    rmt_del_encoder(nec_encoder->bytes_encoder);
    free(nec_encoder);
    return ESP_OK;
}

/**
 * @brief 复位编码器（把状态机与子编码器都回到初始态）
 *
 * @param encoder 编码器基类指针
 * @return ESP_OK
 *
 * 逻辑流程：
 *   ① 复位 copy/bytes 两个子编码器
 *   ② 把状态机 state 置为 RMT_ENCODING_RESET
 */
static esp_err_t rmt_ir_nec_encoder_reset(rmt_encoder_t *encoder)
{
    rmt_ir_nec_encoder_t *nec_encoder = __containerof(encoder, rmt_ir_nec_encoder_t, base);
    rmt_encoder_reset(nec_encoder->copy_encoder);
    rmt_encoder_reset(nec_encoder->bytes_encoder);
    nec_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

/**
 * @brief 创建 NEC 编码器
 *
 * @param config      编码器配置（含 RMT 分辨率，用于把 us 换算成计数）
 * @param ret_encoder 输出：编码器句柄
 * @return ESP_OK 成功；ESP_ERR_INVALID_ARG 参数为空；ESP_ERR_NO_MEM 内存不足
 *
 * 逻辑流程：
 *   ① 参数校验（config/ret_encoder 非空），分配编码器实例内存
 *   ② 挂载三个回调：encode/del/reset
 *   ③ 创建 copy 编码器（用于引导码/结束码）
 *   ④ 计算引导码与结束码符号：9ms+4.5ms、560us+0x7FFF
 *   ⑤ 配置 bytes 编码器位定义：bit0=560us+560us，bit1=560us+1690us
 *   ⑥ 返回编码器句柄；中途失败统一跳 err 释放已分配资源
 */
esp_err_t rmt_new_ir_nec_encoder(const ir_nec_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder)
{
    esp_err_t ret = ESP_OK;
    rmt_ir_nec_encoder_t *nec_encoder = NULL;
    ESP_GOTO_ON_FALSE(config && ret_encoder, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    nec_encoder = rmt_alloc_encoder_mem(sizeof(rmt_ir_nec_encoder_t));
    ESP_GOTO_ON_FALSE(nec_encoder, ESP_ERR_NO_MEM, err, TAG, "no mem for ir nec encoder");
    nec_encoder->base.encode = rmt_encode_ir_nec;
    nec_encoder->base.del = rmt_del_ir_nec_encoder;
    nec_encoder->base.reset = rmt_ir_nec_encoder_reset;

    rmt_copy_encoder_config_t copy_encoder_config = {};
    ESP_GOTO_ON_ERROR(rmt_new_copy_encoder(&copy_encoder_config, &nec_encoder->copy_encoder), err, TAG, "create copy encoder failed");

    /* 引导码符号：9ms 高（脉冲）+ 4.5ms 低（空闲）；
     * duration 以 RMT 计数为单位，故用 us * resolution / 1000000 换算 */
    nec_encoder->nec_leading_symbol = (rmt_symbol_word_t) {
        .level0 = 1,
        .duration0 = 9000ULL * config->resolution / 1000000,
        .level1 = 0,
        .duration1 = 4500ULL * config->resolution / 1000000,
    };
    /* 结束码符号：560us 高（脉冲）+ 0x7FFF 长低（空闲，标记帧结束） */
    nec_encoder->nec_ending_symbol = (rmt_symbol_word_t) {
        .level0 = 1,
        .duration0 = 560 * config->resolution / 1000000,
        .level1 = 0,
        .duration1 = 0x7FFF,
    };

    /* bytes 编码器位定义：数据 0 = 560us 脉冲 + 560us 空闲；
     * 数据 1 = 560us 脉冲 + 1690us 空闲（NEC 标准） */
    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = 560 * config->resolution / 1000000,
            .level1 = 0,
            .duration1 = 560 * config->resolution / 1000000,
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = 560 * config->resolution / 1000000,
            .level1 = 0,
            .duration1 = 1690 * config->resolution / 1000000,
        },
    };
    ESP_GOTO_ON_ERROR(rmt_new_bytes_encoder(&bytes_encoder_config, &nec_encoder->bytes_encoder), err, TAG, "create bytes encoder failed");

    *ret_encoder = &nec_encoder->base;
    return ESP_OK;
err:
    /* 出错路径：释放已分配的子编码器与实例，避免泄漏 */
    if (nec_encoder) {
        if (nec_encoder->bytes_encoder) rmt_del_encoder(nec_encoder->bytes_encoder);
        if (nec_encoder->copy_encoder) rmt_del_encoder(nec_encoder->copy_encoder);
        free(nec_encoder);
    }
    return ret;
}
