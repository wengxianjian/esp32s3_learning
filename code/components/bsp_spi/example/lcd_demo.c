/**
 * @file lcd_demo.c
 * @brief SPI LCD（ILI9341）实验示例（对应指南第二十二章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 初始化 SPI2 总线并对 ILI9341 做完整上电初始化（含背光开启、清屏）；
 * 2) 进入死循环，依次用 8 种颜色整屏填充，每次间隔约 0.8 秒；
 * 3) 屏幕上即可看到背景色循环切换：红→绿→蓝→黄→青→品红→白→黑。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "SPI LCD (ILI9341, color fill)" 这个 demo（对应宏 CONFIG_DEMO_LCD），
 * app_main() 就会调用 bsp_lcd_demo()。
 * 硬件上无需额外跳线（RST/背光由板载 XL9555 控制）。
 */

#include "bsp_lcd.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 日志标签：本示例打印的日志会显示为 "lcd_demo" */
static const char *TAG = "lcd_demo";

void bsp_lcd_demo(void)
{
    /* 提示开始 */
    ESP_LOGI(TAG, "LCD demo start");
    /* 初始化 SPI2 + ILI9341。ESP_ERROR_CHECK：初始化失败（返回非 ESP_OK）
     * 会直接打印断言信息并复位，避免带病运行。 */
    ESP_ERROR_CHECK(bsp_lcd_init());

    /* 待循环展示的 8 种 RGB565 颜色（宏定义见 bsp_lcd.h） */
    static const uint16_t colors[] = {
        LCD_COLOR_RED, LCD_COLOR_GREEN, LCD_COLOR_BLUE, LCD_COLOR_YELLOW,
        LCD_COLOR_CYAN, LCD_COLOR_MAGENTA, LCD_COLOR_WHITE, LCD_COLOR_BLACK,
    };
    /* 当前颜色下标，从 0（红色）开始 */
    int i = 0;
    while (1) {
        /* 用当前颜色整屏填充 */
        bsp_lcd_fill(colors[i]);
        /* 串口打印当前颜色值，便于与屏幕对照（%04x 补零的 4 位十六进制） */
        ESP_LOGI(TAG, "fill color 0x%04x", colors[i]);
        /* 下标加 1 并对颜色总数取模，实现 0→7 循环 */
        i = (i + 1) % (sizeof(colors) / sizeof(colors[0]));
        /* 每种颜色停留 800ms（800 个 tick；默认 1 tick = 1ms） */
        vTaskDelay(pdMS_TO_TICKS(800));
    }
}
