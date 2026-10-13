/**
 * @file bsp_ir_tx.c
 * @brief 红外发送板级驱动实现（RMT TX + 38kHz 载波 + NEC 编码，发射脚 REMOTE_OUT=IO8）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 用 ESP32-S3 的 RMT 外设把 NEC 编码的时序"调制"到 38kHz 载波上发射出去：
 *   bsp_ir_tx_init() —— 创建 RMT TX 通道、加载 38kHz 载波、创建 NEC 编码器
 *   bsp_ir_tx_send() —— 发送一帧 NEC（地址 + 命令）
 * 编码器实现见 ir_nec_encoder.c。
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * ① 载波调制：红外遥控统一使用 38kHz 载波承载数据——有载波代表"脉冲"，
 *    无载波代表"空闲"。RMT TX 通道开启 carrier（38kHz、33% 占空比）后，
 *    编码器只需产生"该发脉冲/该空闲"的电平序列，硬件会自动把"脉冲"段
 *    用 38kHz 方波填充。
 * ② NEC 帧结构（由 ir_nec_encoder 生成）：引导码 9ms 脉冲 + 4.5ms 空闲，
 *    随后 16 位地址 + 16 位命令（低位在先，每位 560us 脉冲 + 560/1690us 空闲
 *    表示 0/1），最后 560us 结束脉冲。
 * ③ 数据流：bsp_ir_tx_send() 把 ir_nec_scan_code_t 交给 RMT，
 *    rmt_transmit 调用 NEC 编码器把结构体编码成符号序列，再经载波调制发出。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 发射引脚/分辨率来自 board.h 的 BSP_IR_TX_GPIO、BSP_IR_RESOLUTION_HZ 宏，
 * 本文件不写死引脚号（38kHz 载波频率、占空比为红外通用取值）。
 * 指南章节：第二十七章 红外发送实验。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * - 发射脚 REMOTE_OUT=IO8 与 ADC_IN、LCD 相关脚存在复用，注意不要同时使用。
 * - rmt_transmit 是异步的：调用返回只表示"已入队"，并不等待发完；
 *   连续发送需自行留出间隔（示例中是 2 秒一次）。
 * - 载波必须先通过 rmt_apply_carrier 加载到通道，再使能、再发送。
 */

#include "bsp_ir_tx.h"
#include "ir_nec_encoder.h"
#include "board.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"

/* 日志标签：打印红外发送相关日志时统一显示为 "bsp_ir_tx" */
static const char *TAG = "bsp_ir_tx";

/* RMT 发送通道句柄：由 bsp_ir_tx_init() 创建 */
static rmt_channel_handle_t s_tx_channel = NULL;
/* NEC 编码器句柄：把"地址+命令"编码成 RMT 符号序列 */
static rmt_encoder_handle_t s_nec_encoder = NULL;

/**
 * @brief 初始化红外发送：创建 RMT TX 通道 + 38kHz 载波 + NEC 编码器
 *
 * @return ESP_OK 成功；其它为创建通道/载波/编码器失败的错误码
 *
 * 逻辑流程：
 *   ① 创建 RMT TX 通道：默认时钟源、1MHz 分辨率、64 符号内存块、
 *      发送队列深度 4、绑定发射 GPIO
 *   ② 配置并加载 38kHz 载波（占空比 33%），让"脉冲"段被 38kHz 调制
 *   ③ 创建 NEC 编码器（分辨率与通道一致）
 *   ④ 使能通道
 */
esp_err_t bsp_ir_tx_init(void)
{
    rmt_tx_channel_config_t cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,             /* 默认时钟源 */
        .resolution_hz = BSP_IR_RESOLUTION_HZ,      /* 1MHz 分辨率 → 1 计数 = 1us */
        .mem_block_symbols = 64,                    /* 通道内存块可存 64 个符号 */
        .trans_queue_depth = 4,                     /* 发送队列深度，允许若干事务排队 */
        .gpio_num = BSP_IR_TX_GPIO,                 /* 发射脚 REMOTE_OUT */
    };
    esp_err_t ret = rmt_new_tx_channel(&cfg, &s_tx_channel);
    if (ret != ESP_OK) return ret;

    /* 38kHz 载波、33% 占空比：红外遥控通用调制参数 */
    rmt_carrier_config_t carrier = {
        .duty_cycle = 0.33,
        .frequency_hz = 38000,
    };
    rmt_apply_carrier(s_tx_channel, &carrier);

    /* 创建 NEC 编码器，分辨率与通道保持一致 */
    ir_nec_encoder_config_t enc_cfg = { .resolution = BSP_IR_RESOLUTION_HZ };
    ret = rmt_new_ir_nec_encoder(&enc_cfg, &s_nec_encoder);
    if (ret != ESP_OK) return ret;

    rmt_enable(s_tx_channel);
    ESP_LOGI(TAG, "IR TX init OK (GPIO%d, 38kHz)", BSP_IR_TX_GPIO);
    return ESP_OK;
}

/**
 * @brief 发送一帧 NEC 红外信号
 *
 * @param address 16 位地址码
 * @param command 16 位命令码
 * @return ESP_OK 表示已成功入队发送；其它为发送失败的错误码
 *
 * 逻辑流程：
 *   ① 把地址/命令打包成 ir_nec_scan_code_t 结构体（编码器的输入）
 *   ② loop_count = 0 表示发送一次、不循环
 *   ③ 调用 rmt_transmit，让 NEC 编码器把结构体编码并入队发送
 */
esp_err_t bsp_ir_tx_send(uint16_t address, uint16_t command)
{
    ir_nec_scan_code_t code = { .address = address, .command = command };
    rmt_transmit_config_t cfg = { .loop_count = 0 };
    return rmt_transmit(s_tx_channel, s_nec_encoder, &code, sizeof(code), &cfg);
}
