/**
 * @file bsp_lcd.c
 * @brief SPI LCD 板级驱动实现（SPI2 + ILI9341，320x240，RST/背光走 XL9555）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 封装 SPI2 主机总线与 ILI9341 显示控制器，对外提供三个函数：
 *   bsp_lcd_init()      —— 初始化 XL9555、SPI2 总线、DC 引脚、复位并按
 *                          预置命令表初始化 ILI9341，最后开背光清屏
 *   bsp_lcd_fill_rect() —— 填充指定矩形区域为单一颜色
 *   bsp_lcd_fill()      —— 整屏填充单一颜色（内部调用 fill_rect 全屏）
 * 另有若干 static 辅助函数：lcd_write_cmd/lcd_write_data（命令/数据分时写）、
 * lcd_set_window（设置显存写入窗口）。
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * ① SPI 4 线时序：本屏用 MOSI(主出从入)、SCLK(时钟)、CS(片选) 三根 SPI 信号，
 *    再加一根 DC(数据/命令选择) 线，故称"SPI 4 线"。
 *      - CS：片选，低电平选中；由 SPI 外设按事务自动拉低/拉高，软件不手动控制。
 *      - DC：DC=0 表示接下来发的是"命令"，DC=1 表示发的是"数据"。
 *      - 每次收发都是先 gpio_set_level 切 DC，再 spi_device_polling_transmit 阻塞发送。
 * ② 像素格式为 RGB565：一个像素 2 字节，高字节在前（大端），
 *    位布局 R4~R0 G5~G0 B4~B0（R5 G6 B5），如红色=0xF800、绿色=0x07E0、蓝色=0x001F。
 * ③ ILI9341 初始化序列（ili9341_init_cmds 表）：上电后需按规格书写入一大串
 *    命令来配置电源控制(0xC0/0xC1)、VCM(0xC5/0xC7)、内存访问方向(0x36)、
 *    像素格式(0x3A=0x55 即 16bit)、帧率(0xB1)、gamma(0xE0/0xE1) 等；
 *    其中 0x11 退出睡眠需等 120ms、0x29 才真正开显示。
 * ④ 画点/填色数据流：先用"列地址 0x2A / 页地址 0x2B"设好写入窗口，
 *    再发"写显存 0x2C"，之后连续发送的像素数据会按窗口自动递增地址填充；
 *    窗口坐标均为 16 位，高字节先发。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * SPI 引脚/CS/DC/复位与背光位/液晶尺寸均来自 board.h 的 BSP_SPI2_*、BSP_LCD_* 宏，
 * 本文件不写死数字。复位与背光通过 XL9555（I2C0）扩展 IO 控制，见 bsp_i2c.h。
 * 指南章节：第二十二章 SPI_LCD实验。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * - 依赖 bsp_xl9555_init()：它会先确保 I2C0 已初始化（内部幂等），否则
 *   bsp_i2c_write_reg 会因驱动未安装返回 ESP_ERR_INVALID_STATE。
 * - 硬件复位后必须留够延时（>10ms+120ms），否则控制器可能未就绪导致花屏；
 *   0x11/软件复位 0x01 后同样各需 120ms。
 * - 大块填充分行发送，避免单次 SPI 传输超过 DMA 缓冲上限。
 */

#include "bsp_lcd.h"
#include "bsp_i2c.h"
#include "board.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

/* 日志标签：打印 LCD 相关日志时统一显示为 "bsp_lcd" */
static const char *TAG = "bsp_lcd";

/* SPI 设备句柄：由 bsp_lcd_init() 添加设备后保存，后续收发都依赖它。
 * 初始为 NULL 表示 SPI 设备尚未挂载。 */
static spi_device_handle_t s_spi = NULL;

/* ILI9341 初始化命令表项：
 *   cmd      —— ILI9341 命令码
 *   data[16] —— 该命令后跟随的参数（最多 16 字节）
 *   len      —— 实际参数字节数（0 表示该命令无参数）
 *   delay_ms —— 发送完该命令后需要等待的毫秒数（部分命令需等待内部完成） */
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t len;
    uint8_t delay_ms;
} lcd_init_cmd_t;

/* ILI9341 上电初始化命令序列（照搬规格书/参考例程的推荐值）：
 * 涵盖电源、驱动、方向、像素格式、gamma 等，末尾 0x11 退出睡眠、0x29 开显示。 */
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

/**
 * @brief 向 ILI9341 写一条命令（无参数）
 *
 * 逻辑流程：
 *   ① 把 DC 拉低，告诉屏幕接下来是"命令"
 *   ② 以 8bit 长度把命令字节通过 SPI 发出（阻塞式轮询发送，发完即返回）
 */
static void lcd_write_cmd(uint8_t cmd)
{
    gpio_set_level(BSP_LCD_DC_GPIO, 0);                 /* 命令模式 */
    spi_transaction_t t = {.length = 8, .tx_buffer = &cmd};
    spi_device_polling_transmit(s_spi, &t);
}

/**
 * @brief 向 ILI9341 写一段数据
 *
 * @param data 数据缓冲区首地址
 * @param len  数据字节数
 *
 * 逻辑流程：
 *   ① 若长度为 0 直接返回（无数据可发，避免空事务）
 *   ② 把 DC 拉高，告诉屏幕接下来是"数据"
 *   ③ 以 len*8 位长度把整段数据通过 SPI 一次发出
 */
static void lcd_write_data(const uint8_t *data, size_t len)
{
    if (len == 0) return;
    gpio_set_level(BSP_LCD_DC_GPIO, 1);                 /* 数据模式 */
    spi_transaction_t t = {.length = len * 8, .tx_buffer = data};
    spi_device_polling_transmit(s_spi, &t);
}

/**
 * @brief 设置显存写入窗口（矩形区域），随后写显存的数据即填充到该区域
 *
 * @param x0,y0 窗口左上角坐标
 * @param x1,y1 窗口右下角坐标
 *
 * 逻辑流程：
 *   ① 发命令 0x2A（列地址/Column Address），再发 4 字节参数 = x0、x1 各 16bit（高字节先）
 *   ② 发命令 0x2B（页地址/Page Address），再发 4 字节参数 = y0、y1 各 16bit（高字节先）
 *   ③ 发命令 0x2C（写显存），此时窗口已就绪，后续数据会按窗口自动递增地址填充
 */
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

/**
 * @brief 初始化 SPI2 总线与 ILI9341，并点亮背光、清屏
 *
 * @return ESP_OK 成功；其它为失败（XL9555/SPI/DMA 等出错，串口有日志）
 *
 * 逻辑流程：
 *   ① 初始化 XL9555（复位/背光走它），失败即返回
 *   ② 初始化 SPI2 主机总线（配置 4 根信号线 + 最大传输长度）
 *   ③ 在 SPI2 上挂载 ILI9341 设备（40MHz、模式 0、CS=IO21）
 *   ④ 配置 DC 引脚为输出并内部上拉
 *   ⑤ 通过 XL9555 拉低→拉高复位脚，做硬件复位并留足延时
 *   ⑥ 软件复位 0x01，再按 ili9341_init_cmds 表逐条下发初始化命令
 *   ⑦ 打开背光，并用白色整屏清屏
 */
esp_err_t bsp_lcd_init(void)
{
    /* 1. XL9555（RST + 背光）
     * 复位脚 P12(0x0400) 与背光脚 P13(0x0800) 都接在 XL9555 扩展 IO 上，
     * 故先初始化 XL9555（其内部会确保 I2C0 已安装），失败则无法控制复位/背光。 */
    esp_err_t ret = bsp_xl9555_init();
    if (ret != ESP_OK) { ESP_LOGE(TAG, "xl9555 init fail: %s", esp_err_to_name(ret)); return ret; }

    /* 2. SPI2 总线 */
    spi_bus_config_t buscfg = {
        .miso_io_num = BSP_SPI2_MISO_GPIO,          /* 主机输入，本屏只写不读，按板级宏配置 */
        .mosi_io_num = BSP_SPI2_MOSI_GPIO,          /* 主机输出，数据/命令都走它 */
        .sclk_io_num = BSP_SPI2_SCLK_GPIO,          /* SPI 时钟，由主机产生 */
        .quadwp_io_num = -1,                        /* 不使用 QSPI 的 WP 线，置 -1 */
        .quadhd_io_num = -1,                        /* 不使用 QSPI 的 HD 线，置 -1 */
        .max_transfer_sz = BSP_LCD_WIDTH * BSP_LCD_HEIGHT * 2, /* 最大单次传输 = 整屏像素(2 字节/像素) */
    };
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "spi_bus_initialize fail: %s", esp_err_to_name(ret)); return ret; }

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40 * 1000 * 1000,         /* SCK 40MHz，ILI9341 常用上限，兼顾刷屏速度与稳定 */
        .mode = 0,                                  /* SPI 模式 0：CPOL=0/CPHA=0，ILI9341 支持 */
        .spics_io_num = BSP_LCD_CS_GPIO,            /* 片选 IO21，由 SPI 外设自动拉低/拉高 */
        .queue_size = 7,                            /* 事务队列深度，允许若干事务排队 */
    };
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &s_spi);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "spi_bus_add_device fail: %s", esp_err_to_name(ret)); return ret; }

    /* 3. DC 引脚（命令/数据选择线，普通 GPIO 输出，软件手动控制） */
    gpio_config_t dc = {
        .pin_bit_mask = 1ULL << BSP_LCD_DC_GPIO,    /* 只配置 DC 这一个引脚（位掩码） */
        .mode = GPIO_MODE_OUTPUT,                   /* 输出：软件写高低电平 */
        .pull_up_en = GPIO_PULLUP_ENABLE,           /* 使能内部上拉，避免悬空误判 */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,      /* 不使用下拉 */
        .intr_type = GPIO_INTR_DISABLE,             /* 不需要中断 */
    };
    gpio_config(&dc);

    /* 4. 硬件复位（RST 经 XL9555 输出）：拉低保持 10ms 再释放，随后等 120ms 让控制器稳定 */
    bsp_xl9555_pin_write(BSP_LCD_RST_IO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    bsp_xl9555_pin_write(BSP_LCD_RST_IO, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    /* 5. 软件复位 + 初始化序列 */
    lcd_write_cmd(0x01);                    /* 0x01 = 软件复位命令 */
    vTaskDelay(pdMS_TO_TICKS(120));         /* 复位后需等待内部重初始化完成 */
    for (size_t i = 0; i < sizeof(ili9341_init_cmds) / sizeof(ili9341_init_cmds[0]); i++) {
        lcd_write_cmd(ili9341_init_cmds[i].cmd);                            /* 下发命令码 */
        lcd_write_data(ili9341_init_cmds[i].data, ili9341_init_cmds[i].len);/* 下发该命令的参数 */
        if (ili9341_init_cmds[i].delay_ms) {
            vTaskDelay(pdMS_TO_TICKS(ili9341_init_cmds[i].delay_ms));       /* 该命令要求的等待时间 */
        }
    }

    /* 6. 背光开 + 清屏 */
    bsp_xl9555_pin_write(BSP_LCD_PWR_IO, 1);    /* 背光脚拉高 = 点亮背光 */
    bsp_lcd_fill(LCD_COLOR_WHITE);             /* 整屏刷白，清除上电随机内容 */

    ESP_LOGI(TAG, "ILI9341 init OK (%dx%d)", BSP_LCD_WIDTH, BSP_LCD_HEIGHT);
    return ESP_OK;
}

/**
 * @brief 把矩形区域填充为单一颜色
 *
 * @param x0,y0 左上角坐标
 * @param x1,y1 右下角坐标
 * @param color RGB565 颜色值
 *
 * 逻辑流程：
 *   ① 计算像素总数（先算长宽再相乘，避免溢出时先算面积）
 *   ② 设置写入窗口为矩形区域
 *   ③ 把该颜色按 RGB565 展开成一行像素缓冲（每像素 2 字节，高字节在前）
 *   ④ 逐行把这一行像素发下去，填满整个矩形
 *
 * 注：pixels 仅用于明确表示"共需填充的像素数"，实际填充以行为单位循环完成。
 */
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

/**
 * @brief 整屏填充单一颜色
 *
 * @param color RGB565 颜色值
 *
 * 逻辑流程：
 *   ① 以左上角 (0,0)、右下角 (宽-1,高-1) 调用 fill_rect 覆盖整屏
 */
void bsp_lcd_fill(uint16_t color)
{
    bsp_lcd_fill_rect(0, 0, BSP_LCD_WIDTH - 1, BSP_LCD_HEIGHT - 1, color);
}
