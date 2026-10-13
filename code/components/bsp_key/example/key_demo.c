/**
 * @file key_demo.c
 * @brief KEY 实验示例（对应指南第十一章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 初始化按键（BOOT 键接 IO0，低电平有效）与 LED（接 IO1，低电平点亮）；
 * 2) 在主循环里用 bsp_key_scan() 轮询按键，检测到按下就把 LED 状态翻转一次；
 * 3) 每轮循环打印一条日志，串口可看到按键事件。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "KEY" 这个 demo（对应宏 CONFIG_DEMO_KEY），
 * app_main() 就会调用 bsp_key_demo()。
 */

#include "bsp_key.h"
#include "bsp_led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 日志标签：本示例打印的日志会显示为 "key_demo" */
static const char *TAG = "key_demo";

/**
 * @brief KEY 示例入口：轮询按键，按下一次翻转一次 LED
 *
 * 逻辑流程：
 *   ① 打印开始提示
 *   ② 初始化按键 GPIO 与 LED GPIO
 *   ③ 死循环：轮询 bsp_key_scan(0)（不支持连按），检测到按下则翻转 LED，
 *      每轮延时 10ms 以降低 CPU 占用
 */
void bsp_key_demo(void)
{
    /* 提示开始，说明操作方式（按 BOOT 键翻转 LED） */
    ESP_LOGI(TAG, "KEY demo start: press BOOT key to toggle LED");
    /* 初始化按键与 LED；ESP_ERROR_CHECK 在失败时断言复位，避免带病运行 */
    ESP_ERROR_CHECK(bsp_key_init());
    ESP_ERROR_CHECK(bsp_led_init());

    while (1) {
        /* 轮询扫描，参数 0 表示不支持连按：一次按下只返回一次，
         * 内部已含 10ms 软件消抖，无需外部再去抖 */
        if (bsp_key_scan(0)) {
            bsp_led_toggle();          /* 检测到按下：翻转 LED 亮灭状态 */
            ESP_LOGI(TAG, "key pressed, LED toggled");
        }
        vTaskDelay(pdMS_TO_TICKS(10)); /* 每 10ms 轮询一次，降低 CPU 占用 */
    }
}
