/**
 * @file ir_tx_demo.c
 * @brief 红外发送实验示例（对应指南第二十七章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 初始化 RMT TX 通道 + 38kHz 载波 + NEC 编码器（发射脚 REMOTE_OUT=IO8）；
 * 2) 进入死循环，每 2 秒发送一帧 NEC（地址 0x0440、命令 0x3003）；
 * 3) 每发一帧打印一次发送内容，串口可看到周期性 "sent Address=... Command=..."。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "Infrared TX (NEC encode)" 这个 demo
 * （对应宏 CONFIG_DEMO_IR_TX），app_main() 就会调用 bsp_ir_tx_demo()。
 * 硬件：用手机摄像头对着红外发射管可看到紫光闪烁；若用另一块板/接收头
 * 按第二十六章示例解码，可收到 0x0440/0x3003。
 * 注意发射脚与 ADC_IN、部分 LCD 引脚复用，不能同时使用。
 */

#include "bsp_ir_tx.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 日志标签：本示例打印的日志会显示为 "ir_tx_demo" */
static const char *TAG = "ir_tx_demo";

void bsp_ir_tx_demo(void)
{
    /* 提示开始 */
    ESP_LOGI(TAG, "IR TX demo start");
    /* 初始化 RMT TX + 38kHz 载波 + NEC 编码器。ESP_ERROR_CHECK：失败会打印断言并复位 */
    ESP_ERROR_CHECK(bsp_ir_tx_init());

    /* 固定发送的地址与命令（示例值，可按需修改） */
    uint16_t address = 0x0440;
    uint16_t command = 0x3003;
    while (1) {
        /* 发送一帧 NEC；ESP_ERROR_CHECK：发送失败会打印断言并复位 */
        ESP_ERROR_CHECK(bsp_ir_tx_send(address, command));
        /* 打印本帧内容，便于与接收端/摄像头观察结果对照 */
        ESP_LOGI(TAG, "sent Address=0x%04X Command=0x%04X", address, command);
        /* 每 2 秒发一帧（发送是异步的，此处间隔给足发送时间） */
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
