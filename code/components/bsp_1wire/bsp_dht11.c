/**
 * @file bsp_dht11.c
 * @brief DHT11 单总线温湿度传感器板级驱动实现（1-Wire，DQ=IO0）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 在单根数据线上用软件精确时序读取 DHT11 的温湿度，对外提供两个函数：
 *   bsp_dht11_init() —— 把 DQ 配成"开漏输入输出 + 上拉"，并复位自检一次
 *   bsp_dht11_read() —— 完整读一帧 5 字节数据，校验后返回温湿度整数部分
 * 另有 static 辅助：dht11_reset（起始信号）、dht11_check（判断应答）、
 * dht11_read_bit/read_byte（读位/读字节）。
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * DHT11 与 DS18B20 一样是单总线器件，DQ 空闲为高，主机与从机分时驱动。
 *   ① 起始信号：主机拉低 ≥18ms（代码用 25ms，用 vTaskDelay 实现），
 *      再释放并保持高 10~35us（代码用 30us），通知器件准备发送数据。
 *   ② 应答：DHT11 先拉低约 83us 表示收到，再拉高约 87us 表示即将开始发数据；
 *      若这 200us 内电平没按预期变化，就判为"无应答/器件不在线"。
 *   ③ 每个 bit：都以 50us 低电平开始，随后高电平长度区分数据——
 *      高电平 26~28us 表示 0，高电平 70us 表示 1；采样点放在拉高后 40us，
 *      此时读到的电平高低即为该位（40us 处 0 已变低、1 仍为高）。
 *   ④ 一帧 40bit，共 5 字节：湿度整数、湿度小数、温度整数、温度小数、校验和；
 *      校验和 = 前 4 字节之和对 256 取余。
 *   ⑤ 数据位为高位在先（MSB first）。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 数据引脚来自 board.h 的 BSP_1WIRE_GPIO 宏（本板 IO0），本文件不写死引脚号；
 * 该脚与 DS18B20 共用，需跳线帽接 WIRE_DQ。
 * 指南章节：第三十章 DHT11 实验。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * - DQ 用开漏 + 上拉，既能输出起始信号又能读入数据。
 * - DHT11 转换较慢，两次读数间隔需 > 1s（上电后也需等约 1s 再读第一次），
 *   否则会读到 0 或校验错误。
 * - 读位的微秒级时序不能被长时间打断，本教学实现未加临界区保护。
 * - 本驱动只返回温湿度整数部分（小数部分 DHT11 通常为 0）。
 */

#include "bsp_dht11.h"
#include "board.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

/* 日志标签：打印 DHT11 相关日志时统一显示为 "bsp_dht11" */
static const char *TAG = "bsp_dht11";

/* 读 DQ 电平的简写宏：DQ 为真表示总线为高 */
#define DQ gpio_get_level(BSP_1WIRE_GPIO)

/**
 * @brief 把 DQ 引脚输出指定电平（开漏输出，1 表示释放总线由上拉拉高）
 *
 * @param level 0 = 拉低，1 = 释放/输出高
 */
static void dq_out(int level)
{
    gpio_set_level(BSP_1WIRE_GPIO, level);
}

/**
 * @brief 发送起始信号：拉低 25ms 后释放并保持高 30us
 *
 * 逻辑流程：
 *   ① 主机拉低总线 25ms（远超要求的 ≥18ms，确保器件识别到起始信号）
 *   ② 释放总线并保持高 30us（落在要求的 10~35us 内）
 */
static void dht11_reset(void)
{
    dq_out(0);
    vTaskDelay(pdMS_TO_TICKS(25));   /* 拉低至少 18ms */
    dq_out(1);
    esp_rom_delay_us(30);            /* 主机拉高 10~35us */
}

/**
 * @brief 等待并判断 DHT11 的应答信号
 *
 * @return 0 = 应答正常；1 = 超时/无应答
 *
 * 逻辑流程：
 *   ① 等待总线被 DHT11 拉低（约 83us），最多等 100us，超时则判失败
 *   ② 等待总线被 DHT11 拉高（约 87us），最多等 100us，超时则判失败
 */
static int dht11_check(void)
{
    uint8_t retry = 0;
    while (DQ && retry < 100) {      /* 等 DHT11 拉低 ~83us */
        retry++;
        esp_rom_delay_us(1);
    }
    if (retry >= 100) return 1;
    retry = 0;
    while (!DQ && retry < 100) {     /* 等 DHT11 拉高 ~87us */
        retry++;
        esp_rom_delay_us(1);
    }
    return (retry >= 100) ? 1 : 0;
}

/**
 * @brief 读 1 位数据
 *
 * @return 该位值（0 或 1）
 *
 * 逻辑流程：
 *   ① 先等总线被拉低（每个 bit 起始的 50us 低电平）
 *   ② 再等高电平开始（低电平结束）
 *   ③ 从高电平开始处等 40us 再采样：
 *      此时高电平若还在 → 数据 1（高约 70us）；已变低 → 数据 0（高约 26us）
 */
static uint8_t dht11_read_bit(void)
{
    uint8_t retry = 0;
    while (DQ && retry < 100) {      /* 等总线拉低 */
        retry++;
        esp_rom_delay_us(1);
    }
    retry = 0;
    while (!DQ && retry < 100) {     /* 等高电平开始 */
        retry++;
        esp_rom_delay_us(1);
    }
    esp_rom_delay_us(40);            /* 40us 后采样 */
    return DQ ? 1 : 0;               /* 高=1，低=0 */
}

/**
 * @brief 读一个字节（高位在先，MSB first）
 *
 * @return 读到的字节
 *
 * 逻辑流程：
 *   ① 循环 8 次，每次先左移一位腾出最低位
 *   ② 把新读到的位填入最低位
 */
static uint8_t dht11_read_byte(void)
{
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        data <<= 1;
        data |= dht11_read_bit();
    }
    return data;
}

/**
 * @brief 初始化 DQ 引脚并做一次自检（起始 + 应答判断）
 *
 * @return ESP_OK 成功；ESP_ERR_NOT_FOUND 未检测到 DHT11；其它为 GPIO 配置失败
 *
 * 逻辑流程：
 *   ① 把 DQ 配置为开漏输入输出 + 上拉
 *   ② 发起始信号并用 dht11_check 判断是否有应答，无应答返回 NOT_FOUND
 *   ③ 有应答则打印初始化信息（含引脚号）
 */
esp_err_t bsp_dht11_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << BSP_1WIRE_GPIO,     /* 只配置 DQ 这一个引脚 */
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,          /* 开漏输入输出：可读可写，适合单总线 */
        .pull_up_en = GPIO_PULLUP_ENABLE,           /* 使能内部上拉，释放总线时回到高电平 */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,      /* 不使用下拉 */
        .intr_type = GPIO_INTR_DISABLE,             /* 不需要中断 */
    };
    esp_err_t ret = gpio_config(&cfg);
    if (ret != ESP_OK) return ret;

    dht11_reset();
    if (dht11_check() != 0) {
        ESP_LOGW(TAG, "DHT11 not present");
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "DHT11 init OK (DQ=GPIO%d)", BSP_1WIRE_GPIO);
    return ESP_OK;
}

/**
 * @brief 读取一次温湿度（整数部分）
 *
 * @param temp 输出：温度整数（°C）
 * @param humi 输出：湿度整数（%RH）
 * @return ESP_OK 成功；ESP_ERR_TIMEOUT 无应答；ESP_ERR_INVALID_CRC 校验错误
 *
 * 逻辑流程：
 *   ① 发起始信号并等待应答，无应答返回 TIMEOUT
 *   ② 连续读 5 个字节到 buf：湿度整数/湿度小数/温度整数/温度小数/校验和
 *   ③ 校验：前 4 字节之和（按 8bit 截断）应等于第 5 字节，不符返回 INVALID_CRC
 *   ④ 把湿度、温度整数部分写入出参
 */
esp_err_t bsp_dht11_read(uint8_t *temp, uint8_t *humi)
{
    uint8_t buf[5] = {0};

    dht11_reset();
    if (dht11_check() != 0) {
        ESP_LOGW(TAG, "DHT11 no response");
        return ESP_ERR_TIMEOUT;
    }
    for (int i = 0; i < 5; i++) {
        buf[i] = dht11_read_byte();
    }
    if ((uint8_t)(buf[0] + buf[1] + buf[2] + buf[3]) != buf[4]) {
        ESP_LOGW(TAG, "DHT11 checksum error");
        return ESP_ERR_INVALID_CRC;
    }
    *humi = buf[0];
    *temp = buf[2];
    return ESP_OK;
}
