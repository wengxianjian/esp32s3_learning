/**
 * @file oled_demo.c
 * @brief SSD1306 OLED 显示实验示例（对应指南第二十一章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 初始化 OLED（I2C1 + DC 引脚 + SSD1306 配置）；
 * 2) 在第 0/2/4 页显示三行固定文字；
 * 3) 进入死循环，在第 6 页显示一个每秒自增的计数，观察画面刷新。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * menuconfig 选 "OLED"（宏 CONFIG_DEMO_OLED），
 * app_main() 调用 bsp_oled_demo()。
 */

#include "bsp_oled.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

/* 日志标签：本示例日志显示为 "oled_demo" */
static const char *TAG = "oled_demo";

/**
 * @brief OLED 实验入口：显示标题并循环刷新计数
 *
 * 逻辑流程：
 *   ① 初始化 OLED
 *   ② 在第 0/2/4 页各显示一行标题文字
 *   ③ 死循环：每秒把计数格式化后显示在第 6 页
 */
void bsp_oled_demo(void)
{
    ESP_LOGI(TAG, "OLED demo start");
    /* 初始化 OLED（失败会断言复位，避免带病运行） */
    ESP_ERROR_CHECK(bsp_oled_init());

    /* 三行标题：x=0 起始列，y 依次取 0/2/4
     * （y 是页号，8x16 字体每行占 2 页，故 4 行分别用页 0/2/4/6） */
    bsp_oled_show_string(0, 0, "ESP32-S3 OLED");
    bsp_oled_show_string(0, 2, "ATK-DNESP32S3");
    bsp_oled_show_string(0, 4, "I2C Test");

    uint32_t cnt = 0;       /* 计数值，每秒 +1 */
    char buf[32];           /* 格式化缓冲区 */
    while (1) {
        /* 把计数格式化成字符串（%lu 配 unsigned long，显式转换以保证跨平台正确） */
        snprintf(buf, sizeof(buf), "cnt:%lu", (unsigned long)cnt++);
        bsp_oled_show_string(0, 6, buf);        /* 第 6 页显示计数 */
        vTaskDelay(pdMS_TO_TICKS(1000));        /* 每 1 秒刷新一次 */
    }
}
