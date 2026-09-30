#include "bsp_lcd.h"
#include "bsp_i2c.h"
#include "board.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "bsp_lcd";

static spi_device_handle_t s_spi = NULL;

/* ILI9341 初始化命令表 */
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t len;
    uint8_t delay_ms;
} lcd_init_cmd_t;

static const lcd_init_cmd_t ili9341_init_cmds[] = {
    {0xCF, {0x00, 0xC1, 0x30}, 3, 0},
    {0xED, {0x64, 0x03, 0x12, 0x81}, 4, 0},
    {0xE8, {0x85, 0x10, 0x78}, 3, 0},
    {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5, 0},
    {0xF7, {0x20}, 1, 0},
    {0xEA, {0x00, 0x00}, 2, 0},
    {0xC0, {0x23}, 1, 0},        /* 电源控制 1 */
    {0xC1, {0x10}, 1, 0},        /* 电源控制 2 */
    {0xC5, {0x3E, 0x28}, 2, 0},  /* VCM 控制 */
    {0xC7, {0x86}, 1, 0},        /* VCM 控制 2 */
    {0x36, {0x48}, 1, 0},        /* 内存访问控制（横屏） */
    {0x3A, {0x55}, 1, 0},        /* 像素格式 16bit */
    {0xB1, {0x00, 0x18}, 2, 0},  /* 帧率控制 */
    {0xB6, {0x08, 0x82, 0x27}, 3, 0}, /* 显示功能控制 */
    {0xF2, {0x00}, 1, 0},        /* 关闭 3G gamma */
    {0x26, {0x01}, 1, 0},        /* gamma 曲线 */
    {0xE0, {0x0F,0x31,0x2B,0x0C,0x0E,0x08,0x4E,0xF1,0x37,0x07,0x10,0x03,0x0E,0x09,0x00}, 15, 0}, /* 正 gamma */
    {0xE1, {0x00,0x0E,0x14,0x03,0x11,0x07,0x31,0xC1,0x48,0x08,0x0F,0x0C,0x31,0x36,0x0F}, 15, 0}, /* 负 gamma */
    {0x11, {0}, 0, 120},         /* 退出睡眠 */
    {0x29, {0}, 0, 0},           /* 开显示 */
};

static void lcd_write_cmd(uint8_t cmd)
{
    gpio_set_level(BSP_LCD_DC_GPIO, 0);                 /* 命令模式 */
    spi_transaction_t t = {.length = 8, .tx_buffer = &cmd};
    spi_device_polling_transmit(s_spi, &t);
}

static void lcd_write_data(const uint8_t *data, size_t len)
{
    if (len == 0) return;
    gpio_set_level(BSP_LCD_DC_GPIO, 1);                 /* 数据模式 */
    spi_transaction_t t = {.length = len * 8, .tx_buffer = data};
    spi_device_polling_transmit(s_spi, &t);
}

static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t d[4];
    lcd_write_cmd(0x2A);   /* 列地址 */
    d[0] = x0 >> 8; d[1] = x0 & 0xFF; d[2] = x1 >> 8; d[3] = x1 & 0xFF;
    lcd_write_data(d, 4);
    lcd_write_cmd(0x2B);   /* 页地址 */
    d[0] = y0 >> 8; d[1] = y0 & 0xFF; d[2] = y1 >> 8; d[3] = y1 & 0xFF;
    lcd_write_data(d, 4);
    lcd_write_cmd(0x2C);   /* 写内存 */
}

esp_err_t bsp_lcd_init(void)
{
    /* 1. XL9555（RST + 背光） */
    esp_err_t ret = bsp_xl9555_init();
    if (ret != ESP_OK) { ESP_LOGE(TAG, "xl9555 init fail: %s", esp_err_to_name(ret)); return ret; }

    /* 2. SPI2 总线 */
    spi_bus_config_t buscfg = {
        .miso_io_num = BSP_SPI2_MISO_GPIO,
        .mosi_io_num = BSP_SPI2_MOSI_GPIO,
        .sclk_io_num = BSP_SPI2_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = BSP_LCD_WIDTH * BSP_LCD_HEIGHT * 2,
    };
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "spi_bus_initialize fail: %s", esp_err_to_name(ret)); return ret; }

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = BSP_LCD_CS_GPIO,
        .queue_size = 7,
    };
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &s_spi);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "spi_bus_add_device fail: %s", esp_err_to_name(ret)); return ret; }

    /* 3. DC 引脚 */
    gpio_config_t dc = {
        .pin_bit_mask = 1ULL << BSP_LCD_DC_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&dc);

    /* 4. 硬件复位 */
    bsp_xl9555_pin_write(BSP_LCD_RST_IO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    bsp_xl9555_pin_write(BSP_LCD_RST_IO, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    /* 5. 软件复位 + 初始化序列 */
    lcd_write_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(120));
    for (size_t i = 0; i < sizeof(ili9341_init_cmds) / sizeof(ili9341_init_cmds[0]); i++) {
        lcd_write_cmd(ili9341_init_cmds[i].cmd);
        lcd_write_data(ili9341_init_cmds[i].data, ili9341_init_cmds[i].len);
        if (ili9341_init_cmds[i].delay_ms) {
            vTaskDelay(pdMS_TO_TICKS(ili9341_init_cmds[i].delay_ms));
        }
    }

    /* 6. 背光开 + 清屏 */
    bsp_xl9555_pin_write(BSP_LCD_PWR_IO, 1);
    bsp_lcd_fill(LCD_COLOR_WHITE);

    ESP_LOGI(TAG, "ILI9341 init OK (%dx%d)", BSP_LCD_WIDTH, BSP_LCD_HEIGHT);
    return ESP_OK;
}

void bsp_lcd_fill_rect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    uint32_t pixels = (uint32_t)(x1 - x0 + 1) * (y1 - y0 + 1);
    lcd_set_window(x0, y0, x1, y1);

    /* 按行分块填充，避免超大单次传输 */
    static uint8_t row[320 * 2];
    uint16_t w = x1 - x0 + 1;
    for (uint16_t i = 0; i < w; i++) {
        row[i * 2] = color >> 8;
        row[i * 2 + 1] = color & 0xFF;
    }
    for (uint16_t r = y0; r <= y1; r++) {
        lcd_write_data(row, w * 2);
    }
    (void)pixels;
}

void bsp_lcd_fill(uint16_t color)
{
    bsp_lcd_fill_rect(0, 0, BSP_LCD_WIDTH - 1, BSP_LCD_HEIGHT - 1, color);
}
