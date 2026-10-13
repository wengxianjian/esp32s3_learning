/**
 * @file xl9555_demo.c
 * @brief XL9555 IO 扩展实验示例（对应指南第十九章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 初始化 I2C0 总线与 XL9555（把各 IO 的方向配好）；
 * 2) 进入死循环，每 1 秒翻转一次蜂鸣器（XL9555 的 P03），并串口打印 ON/OFF；
 * 3) 演示如何用 XL9555 扩展出来的 IO 控制板载蜂鸣器。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * menuconfig 选 "XL9555"（宏 CONFIG_DEMO_XL9555），
 * app_main() 调用 bsp_xl9555_demo()。
 */

#include "bsp_i2c.h"
#include "board.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 日志标签：本示例日志显示为 "xl9555_demo" */
static const char *TAG = "xl9555_demo";

/**
 * @brief XL9555 实验入口：初始化后循环翻转蜂鸣器
 *
 * 逻辑流程：
 *   ① 打印启动提示
 *   ② 初始化 I2C0，再初始化 XL9555（含方向配置与蜂鸣器初始关闭）
 *   ③ 死循环：每秒翻转蜂鸣器电平并打印当前状态
 */
void bsp_xl9555_demo(void)
{
    ESP_LOGI(TAG, "XL9555 demo start: toggle buzzer via I2C");
    /* 先初始化 I2C0 总线，再初始化 XL9555（其内部也会做 I2C 初始化，这里显式调用更清晰） */
    ESP_ERROR_CHECK(bsp_i2c_init());
    ESP_ERROR_CHECK(bsp_xl9555_init());

    bool beep = false;          /* 蜂鸣器状态：false=关闭，true=开启（对应引脚高电平） */
    while (1) {
        beep = !beep;                                   /* ① 状态取反，形成 ON/OFF 交替 */
        bsp_xl9555_pin_write(BSP_XL9555_BEEP_IO, beep); /* ② 写蜂鸣器引脚（P03，掩码 0x0008） */
        ESP_LOGI(TAG, "buzzer %s", beep ? "ON" : "OFF");/* ③ 打印当前状态 */
        vTaskDelay(pdMS_TO_TICKS(1000));                /* ④ 间隔 1 秒，便于听到清晰的通断 */
    }
}
