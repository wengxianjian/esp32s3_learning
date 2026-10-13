/**
 * @file bsp_ir_rx.c
 * @brief 红外接收板级驱动实现（RMT RX + NEC 解码，接收头 REMOTE_IN=IO2）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 用 ESP32-S3 的 RMT 外设接收红外接收头解调后的数字脉冲，并按 NEC 协议解码：
 *   bsp_ir_rx_init()    —— 创建 RMT RX 通道、注册接收完成回调、启动首次接收
 *   bsp_ir_rx_receive() —— 阻塞等待一帧并解码出地址/命令
 * 另有 static 辅助：in_range（时间容差判断）、parse_logic0/parse_logic1（位判定）、
 * nec_parse_frame（整帧解析）、rx_done_cb（接收完成中断回调）、start_receive（启动接收）。
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * 红外接收头内部已把 38kHz 载波解调掉，输出端给出"有脉冲=低、无脉冲=高"的
 * 数字波形；RMT 以 1MHz 分辨率（1 个计数 = 1us）把高低电平的持续时间录成
 * 一串"符号"（rmt_symbol_word_t：每符号含 level0/duration0、level1/duration1 两段）。
 * NEC 一帧的结构与时序：
 *   ① 引导码：9ms 载波脉冲 + 4.5ms 空闲（代码 NEC_LEADING_0/1 = 9000/4500us）；
 *   ② 32 位数据：先 16 位地址，再 16 位命令，均为低位在先；
 *   ③ 位编码：每位都是 560us 脉冲开始——后随 560us 空闲表示 0
 *      （NEC_ZERO_0/1）；后随 1690us 空闲表示 1（NEC_ONE_0/1）。
 * 解码时对每段实测时长与标准值做容差比较（BSP_IR_NEC_DECODE_MARGIN 微秒），
 * 落在 [spec-margin, spec+margin] 内即认为匹配。任一位既不像 0 也不像 1，
 * 或引导码不符，则整帧判为失败。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 引脚/分辨率/容差来自 board.h 的 BSP_IR_RX_GPIO、BSP_IR_RESOLUTION_HZ、
 * BSP_IR_NEC_DECODE_MARGIN 宏，本文件不写死数字（NEC 标准时序常量除外）。
 * 指南章节：第二十六章 红外接收实验。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * - 接收脚 REMOTE_IN=IO2 与 SD 卡 TF_CS 复用，二者不能同时使用。
 * - 接收完成回调在 ISR 上下文执行，只做"入队"这一件轻量事，解析放到任务里做。
 * - 队列长度只有 1：上一帧未取走时新帧会丢弃，属正常（遥控按键节奏较慢）。
 * - 每收到一帧都要重新调用 start_receive() 才会继续接收下一帧。
 */

#include "bsp_ir_rx.h"
#include "board.h"
#include "driver/rmt_rx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"

/* 日志标签：打印红外接收相关日志时统一显示为 "bsp_ir_rx" */
static const char *TAG = "bsp_ir_rx";

/* RMT 接收通道句柄：由 bsp_ir_rx_init() 创建 */
static rmt_channel_handle_t s_rx_channel = NULL;
/* 接收完成事件队列：ISR 向它投递"一帧接收完成"事件，任务侧阻塞读取 */
static QueueHandle_t s_rx_queue = NULL;

/* NEC 协议标准时序（单位 us）；RMT 分辨率 1MHz 时 1 计数 = 1us：
 *   引导码：9ms 脉冲 + 4.5ms 空闲
 *   数据位 0：560us 脉冲 + 560us 空闲
 *   数据位 1：560us 脉冲 + 1690us 空闲 */
#define NEC_LEADING_0   9000
#define NEC_LEADING_1   4500
#define NEC_ZERO_0      560
#define NEC_ZERO_1      560
#define NEC_ONE_0       560
#define NEC_ONE_1       1690

/**
 * @brief 判断实测时长 d 是否在标准值 spec 的容差范围内
 *
 * @param d    实测时长（单位 us）
 * @param spec 标准时长（单位 us）
 * @return true = 在 [spec-容差, spec+容差] 内；false = 超出容差
 *
 * 容差取 board.h 的 BSP_IR_NEC_DECODE_MARGIN（本板 200us），
 * 用于容忍遥控器晶振误差与接收头边沿抖动。
 */
static inline bool in_range(uint32_t d, uint32_t spec)
{
    return (d < spec + BSP_IR_NEC_DECODE_MARGIN) && (d > spec - BSP_IR_NEC_DECODE_MARGIN);
}

/**
 * @brief 判断一个 RMT 符号是否表示 NEC 数据位 0
 *
 * @param s RMT 符号（含两段时长）
 * @return true = 0（560us 脉冲 + 560us 空闲）
 */
static bool parse_logic0(rmt_symbol_word_t *s)
{
    return in_range(s->duration0, NEC_ZERO_0) && in_range(s->duration1, NEC_ZERO_1);
}

/**
 * @brief 判断一个 RMT 符号是否表示 NEC 数据位 1
 *
 * @param s RMT 符号（含两段时长）
 * @return true = 1（560us 脉冲 + 1690us 空闲）
 */
static bool parse_logic1(rmt_symbol_word_t *s)
{
    return in_range(s->duration0, NEC_ONE_0) && in_range(s->duration1, NEC_ONE_1);
}

/**
 * @brief 解析一整帧 NEC 信号，输出地址与命令
 *
 * @param symbols  RMT 录下的符号数组（首个为引导码）
 * @param address  输出：16 位地址
 * @param command  输出：16 位命令
 * @return true = 解析成功；false = 引导码或任一数据位不符
 *
 * 逻辑流程：
 *   ① 校验第 1 个符号是否为引导码（9ms + 4.5ms），不是则整帧失败
 *   ② 指针后移一位，准备读 32 位数据
 *   ③ 先循环 16 次读地址，再循环 16 次读命令；均为低位在先：
 *      该位是 1 则置位 (1<<i)，是 0 则清零该位，两者都不像则失败
 *   ④ 把解析结果写入出参并返回成功
 */
static bool nec_parse_frame(rmt_symbol_word_t *symbols, uint16_t *address, uint16_t *command)
{
    rmt_symbol_word_t *cur = symbols;
    if (!in_range(cur->duration0, NEC_LEADING_0) || !in_range(cur->duration1, NEC_LEADING_1)) {
        return false;
    }
    cur++;
    uint16_t addr = 0, cmd = 0;
    for (int i = 0; i < 16; i++) {
        if (parse_logic1(cur)) addr |= 1 << i;
        else if (parse_logic0(cur)) addr &= ~(1 << i);
        else return false;
        cur++;
    }
    for (int i = 0; i < 16; i++) {
        if (parse_logic1(cur)) cmd |= 1 << i;
        else if (parse_logic0(cur)) cmd &= ~(1 << i);
        else return false;
        cur++;
    }
    *address = addr;
    *command = cmd;
    return true;
}

/**
 * @brief RMT 接收完成中断回调（ISR 上下文）
 *
 * @param channel   触发回调的 RMT 通道（未使用）
 * @param edata     接收完成事件数据（含录下的符号）
 * @param user_data 用户上下文，这里传入接收事件队列句柄
 * @return 是否需要在退出 ISR 后触发任务切换（有高优先级任务被唤醒时返回 true）
 *
 * 逻辑流程：
 *   ① 用 xQueueSendFromISR 把事件投递到队列（本函数是 ISR，必须用 FromISR 版本）
 *   ② 返回"是否需要任务切换"标志
 */
static bool rx_done_cb(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data)
{
    BaseType_t w = pdFALSE;
    xQueueSendFromISR((QueueHandle_t)user_data, edata, &w);
    return w == pdTRUE;
}

/**
 * @brief 启动一次接收（准备接收下一帧）
 *
 * 逻辑流程：
 *   ① 配置接收条件：最短 1250ns（过滤载波毛刺/短噪声）、
 *      最长 12ms（超过该时长的空闲视为一帧结束）
 *   ② 准备一个 64 符号的缓冲并调用 rmt_receive 启动接收
 */
static void start_receive(void)
{
    rmt_receive_config_t cfg = {
        .signal_range_min_ns = 1250,
        .signal_range_max_ns = 12000000,
    };
    rmt_symbol_word_t symbols[64];
    rmt_receive(s_rx_channel, symbols, sizeof(symbols), &cfg);
}

/**
 * @brief 初始化红外接收：创建 RMT RX 通道、注册回调并启动首次接收
 *
 * @return ESP_OK 成功；其它为创建通道失败的错误码
 *
 * 逻辑流程：
 *   ① 创建 RMT RX 通道：默认时钟源、1MHz 分辨率、64 符号内存块、绑定接收 GPIO
 *   ② 创建长度为 1 的接收事件队列
 *   ③ 注册接收完成回调（把事件投递到该队列）
 *   ④ 使能通道并启动第一次接收
 */
esp_err_t bsp_ir_rx_init(void)
{
    rmt_rx_channel_config_t cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,             /* 默认时钟源 */
        .resolution_hz = BSP_IR_RESOLUTION_HZ,      /* 1MHz 分辨率 → 1 计数 = 1us */
        .mem_block_symbols = 64,                    /* 通道内存块可存 64 个符号 */
        .gpio_num = BSP_IR_RX_GPIO,                 /* 接收脚 REMOTE_IN */
    };
    esp_err_t ret = rmt_new_rx_channel(&cfg, &s_rx_channel);
    if (ret != ESP_OK) return ret;

    s_rx_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    rmt_rx_event_callbacks_t cbs = { .on_recv_done = rx_done_cb };
    rmt_rx_register_event_callbacks(s_rx_channel, &cbs, s_rx_queue);
    rmt_enable(s_rx_channel);
    start_receive();

    ESP_LOGI(TAG, "IR RX init OK (GPIO%d)", BSP_IR_RX_GPIO);
    return ESP_OK;
}

/**
 * @brief 阻塞等待一帧 NEC 信号并解码
 *
 * @param address    输出：解析出的 16 位地址
 * @param command    输出：解析出的 16 位命令
 * @param timeout_ms 等待超时（毫秒）
 * @return ESP_OK 成功；ESP_ERR_TIMEOUT 超时未收到；ESP_ERR_INVALID_RESPONSE 收到但解码失败
 *
 * 逻辑流程：
 *   ① 从队列取接收完成事件，超时则返回 TIMEOUT
 *   ② 默认返回 INVALID_RESPONSE；调用 nec_parse_frame 解析，成功则改为 ESP_OK
 *   ③ 重新启动接收，准备下一帧
 */
esp_err_t bsp_ir_rx_receive(uint16_t *address, uint16_t *command, uint32_t timeout_ms)
{
    rmt_rx_done_event_data_t rx_data;
    if (xQueueReceive(s_rx_queue, &rx_data, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t ret = ESP_ERR_INVALID_RESPONSE;
    if (nec_parse_frame(rx_data.received_symbols, address, command)) {
        ret = ESP_OK;
    }
    start_receive();   /* 继续接收下一帧 */
    return ret;
}
