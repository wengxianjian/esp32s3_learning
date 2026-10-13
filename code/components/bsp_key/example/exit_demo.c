/**
 * @file exit_demo.c
 * @brief EXIT 实验示例（外部中断方式，对应指南第十一章中断篇）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 初始化按键外部中断（BOOT 键接 IO0，下降沿触发）与 LED（接 IO1，低电平点亮）；
 * 2) 按键按下时 ISR 只置一个标志，主循环通过 bsp_key_intr_triggered() 读取该标志，
 *    有触发就把 LED 翻转一次，体现「中断投递事件、任务处理」的编程模型；
 * 3) 对比 key_demo 的轮询方式，本方式无需在循环里反复读引脚电平。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "EXIT" 这个 demo（对应宏 CONFIG_DEMO_EXIT），
 * app_main() 就会调用 bsp_exit_demo()。
 */

#include "bsp_key.h"
#include "bsp_led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 日志标签：本示例打印的日志会显示为 "exit_demo" */
static const char *TAG = "exit_demo";

/**
 * @brief EXIT 示例入口：中断方式检测按键，按下一次翻转一次 LED
 *
 * 逻辑流程：
 *   ① 打印开始提示
 *   ② 初始化按键外部中断与 LED GPIO
 *   ③ 死循环：轮询 bsp_key_intr_triggered()，有触发则翻转 LED，
 *      每轮延时 10ms（ISR 已完成置位，任务只负责处理）
 */
void bsp_exit_demo(void)
{
    /* 提示开始，说明使用中断方式检测按键 */
    ESP_LOGI(TAG, "EXIT demo start: press BOOT key (interrupt) to toggle LED");
    /* 初始化按键中断（含 ISR 注册）与 LED；失败即断言复位 */
    ESP_ERROR_CHECK(bsp_key_intr_init());
    ESP_ERROR_CHECK(bsp_led_init());

    while (1) {
        /* 查询中断是否触发过：有触发返回 true 并自动清标志，保证一次按下只处理一次 */
        if (bsp_key_intr_triggered()) {
            bsp_led_toggle();          /* 翻转 LED 亮灭状态 */
            ESP_LOGI(TAG, "key interrupt triggered, LED toggled");
        }
        vTaskDelay(pdMS_TO_TICKS(10)); /* 每 10ms 检查一次标志，降低 CPU 占用 */
    }
}
