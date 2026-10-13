/**
 * @file ap3216c_demo.c
 * @brief AP3216C 光强/接近/红外实验示例（对应指南第二十五章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 初始化 AP3216C（软复位 + 使能 IR/ALS/PS 三路传感器）；
 * 2) 进入死循环，每 500ms 读一次 IR/ALS/PS 并通过串口打印；
 * 3) 用手遮挡传感器或改变环境光，可看到数值实时变化。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * menuconfig 选 "AP3216C"（宏 CONFIG_DEMO_AP3216C），
 * app_main() 调用 bsp_ap3216c_demo()。
 */

#include "bsp_ap3216c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 日志标签：本示例日志显示为 "ap3216c_demo" */
static const char *TAG = "ap3216c_demo";

/**
 * @brief AP3216C 实验入口：周期读取并打印三路数据
 *
 * 逻辑流程：
 *   ① 初始化 AP3216C（失败断言复位）
 *   ② 死循环：每 500ms 读一次 IR/ALS/PS 并打印
 */
void bsp_ap3216c_demo(void)
{
    ESP_LOGI(TAG, "AP3216C demo start");
    /* 初始化传感器（内部会先初始化 I2C0） */
    ESP_ERROR_CHECK(bsp_ap3216c_init());

    uint16_t ir, als, ps;   /* 三路读数：红外 IR / 环境光 ALS / 接近 PS */
    while (1) {
        /* 一次读回三路数据；失败会断言复位 */
        ESP_ERROR_CHECK(bsp_ap3216c_read(&ir, &als, &ps));
        ESP_LOGI(TAG, "IR=%u  ALS=%u  PS=%u", ir, als, ps);
        vTaskDelay(pdMS_TO_TICKS(500));   /* 大于 112.5ms 的转换间隔 */
    }
}
