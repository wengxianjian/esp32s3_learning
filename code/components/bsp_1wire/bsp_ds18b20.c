/**
 * @file bsp_ds18b20.c
 * @brief DS18B20 单总线温度传感器板级驱动实现（1-Wire，DQ=IO0）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 在单根数据线上用软件精确时序实现 Dallas 1-Wire 协议，对外提供两个函数：
 *   bsp_ds18b20_init()            —— 把 DQ 引脚配成"开漏输入输出 + 上拉"
 *   bsp_ds18b20_read_temperature()—— 复位→发转换命令→等待→读回温度并换算
 * 另有 static 辅助：复位/写位/读位/写字节/读字节，以及 dq_out/dq_in 引脚读写。
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * 单总线（1-Wire）只有一根数据线，主机与从机分时复用：空闲时线上为高，
 * 任何一方拉低一段时间即代表发送一位，时间长度区分 0/1。所有延时都是
 * 微秒级、用 esp_rom_delay_us() 精确产生（不能超过 ~1ms 否则协议超时）。
 *   ① 复位（Reset）：主机拉低 ≥480us 后释放，器件若在线会在 15~60us 内
 *      拉低 60~240us 作为"存在脉冲"（presence pulse），主机据此判断器件是否存在。
 *   ② 写 0：主机拉低 60us 再释放；写 1：主机拉低 2us 后立即释放，靠 60us 时隙内的
 *      高电平时间长短区分。
 *   ③ 读位：主机先拉低 2us 启动读时隙，释放总线，约 15us 后采样线上电平即为该位值，
 *      之后等 45us 结束时隙。
 *   ④ 读温度流程：复位 → SkipROM(0xCC，只有一个器件时跳过寻址) →
 *      Convert(0x44，启动一次温度转换) → 等 750ms（12 位分辨率）→ 复位 →
 *      SkipROM → ReadScratchpad(0xBE，读暂存器) → 读 2 字节(LSB 在前)。
 *   ⑤ 温度换算：原始值是有符号 16bit，低 4 位为小数，1 LSB = 0.0625°C，
 *      即 温度(°C) = raw / 16.0。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 数据引脚来自 board.h 的 BSP_1WIRE_GPIO 宏（本板 IO0），本文件不写死引脚号；
 * 该脚与 DHT11 共用，需跳线帽接 WIRE_DQ。
 * 指南章节：第二十九章 DS18B20 实验。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * - DQ 必须用开漏(GPIO_MODE_INPUT_OUTPUT_OD) + 上拉，才能既输出又读入，
 *   多个器件挂同一条线时才不会互相短路。
 * - 单总线时序对微秒极敏感，读位/写位期间不可被高优先级任务打断太久，
 *   必要时需进临界区；本教学实现未加临界区保护。
 * - 转换期间 (750ms) 用 vTaskDelay 让出 CPU，不要用忙等。
 * - 未接器件时复位检测不到存在脉冲，函数返回 NAN。
 */

#include "bsp_ds18b20.h"
#include "board.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include <math.h>

/* 日志标签：打印 DS18B20 相关日志时统一显示为 "bsp_ds18b20" */
static const char *TAG = "bsp_ds18b20";

/* DS18B20 常用命令码（ROM 命令与功能命令）：
 *   SKIP_ROM    0xCC —— 跳过 ROM 寻址，总线上只有一个器件时可直接使用
 *   CONVERT     0x44 —— 启动一次温度转换（功耗较大，转换期间不要发其他命令）
 *   READ_SCRATCH 0xBE —— 读暂存器（9 字节），前 2 字节即温度原始值 */
#define DS18B20_CMD_SKIP_ROM     0xCC
#define DS18B20_CMD_CONVERT      0x44
#define DS18B20_CMD_READ_SCRATCH 0xBE

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
 * @brief 读入当前 DQ 引脚电平
 *
 * @return 1 = 线上为高，0 = 线上为低
 */
static int dq_in(void)
{
    return gpio_get_level(BSP_1WIRE_GPIO);
}

/**
 * @brief 初始化单总线 GPIO
 *
 * @return ESP_OK 成功；其它为 GPIO 配置失败
 *
 * 逻辑流程：
 *   ① 把 DQ 配置为开漏输入输出（既能主动拉低、又能读回电平）+ 内部上拉
 *   ② 成功则打印初始化信息（含引脚号，便于对照硬件）
 */
esp_err_t bsp_ds18b20_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << BSP_1WIRE_GPIO,     /* 只配置 DQ 这一个引脚 */
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,          /* 开漏输入输出：可读可写，适合单总线 */
        .pull_up_en = GPIO_PULLUP_ENABLE,           /* 使能内部上拉，释放总线时回到高电平 */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,      /* 不使用下拉 */
        .intr_type = GPIO_INTR_DISABLE,             /* 不需要中断 */
    };
    esp_err_t ret = gpio_config(&cfg);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "DS18B20 init OK (DQ=GPIO%d)", BSP_1WIRE_GPIO);
    }
    return ret;
}

/* 复位：拉低 480us 释放，检测存在脉冲 */
/**
 * @brief 单总线复位并检测器件是否存在（存在脉冲）
 *
 * @return 1 = 检测到器件（存在脉冲）；0 = 无器件
 *
 * 逻辑流程：
 *   ① 主机拉低总线 480us（≥480us 的复位脉冲）
 *   ② 释放总线并等 60us，让器件有机会拉低总线
 *   ③ 采样总线电平：0 表示器件拉出了存在脉冲 → 在线
 *   ④ 再等 240us，等存在脉冲结束、总线回到空闲高电平
 */
static int ds18b20_reset(void)
{
    dq_out(0);
    esp_rom_delay_us(480);
    dq_out(1);
    esp_rom_delay_us(60);
    int presence = dq_in();   /* 0 = 器件存在 */
    esp_rom_delay_us(240);
    return presence == 0;
}

/**
 * @brief 写 1 位（用低电平持续时间区分 0/1）
 *
 * @param bit 待写位（0 或 1）
 *
 * 逻辑流程：
 *   ① 写 1：拉低 2us 后立即释放，其余时隙保持高，总时隙 60us 左右
 *   ② 写 0：拉低 60us（占满时隙）后再释放，用长低电平表示 0
 */
static void ds18b20_write_bit(int bit)
{
    if (bit) {
        dq_out(0);
        esp_rom_delay_us(2);
        dq_out(1);
        esp_rom_delay_us(60);
    } else {
        dq_out(0);
        esp_rom_delay_us(60);
        dq_out(1);
        esp_rom_delay_us(2);
    }
}

/**
 * @brief 读 1 位
 *
 * @return 读到的位（0 或 1）
 *
 * 逻辑流程：
 *   ① 主机拉低 2us 启动读时隙，再释放总线交给从机驱动
 *   ② 等 15us（器件应在此时把数据摆到线上），采样电平即为该位
 *   ③ 再等 45us 凑满约 60us 读时隙
 */
static int ds18b20_read_bit(void)
{
    int bit = 0;
    dq_out(0);
    esp_rom_delay_us(2);
    dq_out(1);
    esp_rom_delay_us(15);
    bit = dq_in();
    esp_rom_delay_us(45);
    return bit;
}

/**
 * @brief 写一个字节（低位在前，LSB first）
 *
 * @param data 待写字节
 *
 * 逻辑流程：
 *   ① 循环 8 次，每次取出最低位写出去
 *   ② 右移一位，准备写下一个更高位
 */
static void ds18b20_write_byte(uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        ds18b20_write_bit(data & 0x01);
        data >>= 1;
    }
}

/**
 * @brief 读一个字节（低位在前，LSB first）
 *
 * @return 读到的字节
 *
 * 逻辑流程：
 *   ① 循环 8 次，每次读一位
 *   ② 若该位为 1，则把结果对应比特位置 1
 */
static uint8_t ds18b20_read_byte(void)
{
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        if (ds18b20_read_bit()) {
            data |= (1 << i);
        }
    }
    return data;
}

/**
 * @brief 读取一次温度（摄氏度）
 *
 * @return 温度值（°C），分辨率 0.0625°C；无器件时返回 NAN
 *
 * 逻辑流程：
 *   ① 复位，检测器件是否在线，不在线返回 NAN
 *   ② 发 SkipROM(0xCC) + Convert(0x44)，启动一次温度转换
 *   ③ 等待 750ms（12 位分辨率转换时间）
 *   ④ 再次复位并 SkipROM + ReadScratchpad(0xBE)
 *   ⑤ 读回 2 字节原始值（先低字节 LSB，后高字节 MSB），拼成有符号 16bit
 *   ⑥ 除以 16（即 1 LSB = 0.0625°C）得到摄氏度
 */
float bsp_ds18b20_read_temperature(void)
{
    if (!ds18b20_reset()) {
        ESP_LOGW(TAG, "DS18B20 not present");
        return NAN;
    }
    ds18b20_write_byte(DS18B20_CMD_SKIP_ROM);
    ds18b20_write_byte(DS18B20_CMD_CONVERT);
    vTaskDelay(pdMS_TO_TICKS(750));   /* 12 位转换约 750ms */

    if (!ds18b20_reset()) {
        ESP_LOGW(TAG, "DS18B20 not present (after convert)");
        return NAN;
    }
    ds18b20_write_byte(DS18B20_CMD_SKIP_ROM);
    ds18b20_write_byte(DS18B20_CMD_READ_SCRATCH);

    uint8_t lsb = ds18b20_read_byte();
    uint8_t msb = ds18b20_read_byte();
    int16_t raw = (int16_t)((msb << 8) | lsb);
    return raw / 16.0f;   /* 12 位，0.0625°C/LSB */
}
