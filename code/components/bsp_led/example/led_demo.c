/**
 * @file led_demo.c
 * @brief LED 实验示例（对应指南第十章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 先初始化 LED（用户 LED 接 IO1，低电平点亮）；
 * 2) 然后进入死循环，让 LED 以 500ms 亮、500ms 灭的节奏持续闪烁；
 * 3) 通过串口打印一条开始提示，便于确认当前运行的是哪个 demo。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "LED" 这个 demo（对应宏 CONFIG_DEMO_LED），
 * app_main() 就会调用 bsp_led_demo()。
 */

#include "bsp_led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 日志标签：本示例打印的日志会显示为 "led_demo" */
static const char *TAG = "led_demo";

/**
 * @brief LED 示例入口：初始化后以 500ms 间隔闪烁
 *
 * 逻辑流程：
 *   ① 打印开始提示
 *   ② 初始化 LED GPIO（失败即断言复位）
 *   ③ 死循环：点亮 → 延时 500ms → 熄灭 → 延时 500ms
 */
void bsp_led_demo(void)
{
    /* 提示开始，并说明闪烁周期，方便与实测现象对照 */
    ESP_LOGI(TAG, "LED demo start: blink every 500 ms");
    /* 初始化 LED。ESP_ERROR_CHECK：初始化失败（返回非 ESP_OK）会直接打印
     * 断言信息并复位，避免带病运行。 */
    ESP_ERROR_CHECK(bsp_led_init());

    while (1) {
        bsp_led_set(true);              /* 点亮（on=true，内部按低电平有效换算成 GPIO 输出 0） */
        vTaskDelay(pdMS_TO_TICKS(500)); /* 亮 500ms（默认 1 tick = 1ms） */
        bsp_led_set(false);             /* 熄灭 */
        vTaskDelay(pdMS_TO_TICKS(500)); /* 灭 500ms */
    }
}
