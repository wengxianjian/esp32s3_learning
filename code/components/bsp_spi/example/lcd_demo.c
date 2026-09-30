#include "bsp_lcd.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "lcd_demo";

void bsp_lcd_demo(void)
{
    ESP_LOGI(TAG, "LCD demo start");
    ESP_ERROR_CHECK(bsp_lcd_init());

    static const uint16_t colors[] = {
        LCD_COLOR_RED, LCD_COLOR_GREEN, LCD_COLOR_BLUE, LCD_COLOR_YELLOW,
        LCD_COLOR_CYAN, LCD_COLOR_MAGENTA, LCD_COLOR_WHITE, LCD_COLOR_BLACK,
    };
    int i = 0;
    while (1) {
        bsp_lcd_fill(colors[i]);
        ESP_LOGI(TAG, "fill color 0x%04x", colors[i]);
        i = (i + 1) % (sizeof(colors) / sizeof(colors[0]));
        vTaskDelay(pdMS_TO_TICKS(800));
    }
}
