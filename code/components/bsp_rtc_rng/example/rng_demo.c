/**
 * @file rng_demo.c
 * @brief RNG 实验示例（对应指南第三十一章 RNG）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 进入死循环，每秒读取并打印一个 32 位硬件随机数；
 * 2) 同时打印一个 [1, 100] 闭区间内的随机整数；
 * 3) 每次数值都不同，说明芯片硬件随机数发生器（RNG）工作正常，且无需初始化。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "RNG (random number)"（对应宏 CONFIG_DEMO_RNG），
 * app_main() 就会调用 bsp_rng_demo()。
 */

#include "bsp_rtc_rng.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 日志标签：本示例打印的日志会显示为 "rng_demo" */
static const char *TAG = "rng_demo";

void bsp_rng_demo(void)
{
    ESP_LOGI(TAG, "RNG demo start");

    while (1) {
        /* 取一个 32 位硬件随机数 */
        uint32_t r = bsp_rng_random();
        /* 取一个 [1, 100] 闭区间内的随机整数 */
        int range = bsp_rng_range(1, 100);
        /* 打印结果：r 用无符号长整型 %lu（显式转 unsigned long 以兼容），range 用 %d */
        ESP_LOGI(TAG, "random = %lu, range[1,100] = %d", (unsigned long)r, range);
        /* 每秒刷新一次，便于观察数值持续变化 */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
