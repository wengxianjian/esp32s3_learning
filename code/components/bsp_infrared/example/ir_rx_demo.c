/**
 * @file ir_rx_demo.c
 * @brief 红外接收实验示例（对应指南第二十六章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 初始化 RMT RX 通道，开始监听红外接收头（REMOTE_IN=IO2）；
 * 2) 进入死循环，每次阻塞最多 60 秒等待一帧 NEC 信号；
 * 3) 解码成功则打印该帧的地址与命令（Address=0xXXXX, Command=0xXXXX）。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "Infrared RX (NEC decode)" 这个 demo
 * （对应宏 CONFIG_DEMO_IR_RX），app_main() 就会调用 bsp_ir_rx_demo()。
 * 硬件：把红外遥控器对准板载红外接收头，按下任意键即可看到打印；
 * 注意接收脚与 SD 卡 TF_CS 复用，不能同时使用 SD 卡。
 */

#include "bsp_ir_rx.h"
#include "esp_log.h"

/* 日志标签：本示例打印的日志会显示为 "ir_rx_demo" */
static const char *TAG = "ir_rx_demo";

void bsp_ir_rx_demo(void)
{
    /* 提示开始 */
    ESP_LOGI(TAG, "IR RX demo start");
    /* 初始化 RMT RX 通道并启动首次接收。ESP_ERROR_CHECK：失败会打印断言并复位 */
    ESP_ERROR_CHECK(bsp_ir_rx_init());

    /* 出参缓冲：解析成功后由 bsp_ir_rx_receive 写入地址与命令 */
    uint16_t address, command;
    while (1) {
        /* 阻塞等待一帧，最多 60000ms（60 秒）；超时返回 ESP_ERR_TIMEOUT */
        esp_err_t ret = bsp_ir_rx_receive(&address, &command, 60000);
        if (ret == ESP_OK) {
            /* %04X：补零的 4 位十六进制，便于对照遥控器键码 */
            ESP_LOGI(TAG, "Address=0x%04X, Command=0x%04X", address, command);
        }
    }
}
