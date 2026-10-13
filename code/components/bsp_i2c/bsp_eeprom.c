/**
 * @file bsp_eeprom.c
 * @brief AT24C02 EEPROM 板级驱动实现（I2C0，7 位地址 0x50，容量 256 字节）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 本文件在 I2C0 上读写 AT24C02 这一掉电不丢失的存储器：
 *   bsp_eeprom_write() —— 从指定存储地址起写入一段数据（写完等待内部擦写周期）
 *   bsp_eeprom_read()  —— 从指定存储地址起读出一段数据
 * 二者直接复用 bsp_i2c.c 的寄存器读写原语，把「存储地址」当作寄存器指针传入。
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * EEPROM 是掉电不丢失的存储器，通过 I2C 读写，字节寻址。
 *   写时序：START → 器件地址(W) → 存储地址（字地址） → 数据… → STOP；
 *           器件收到 STOP 后开始内部写入，期间不应答总线，故需延时等待
 *           内部写周期（tWR，约 5~10ms）完成，之后才能发起下一次访问。
 *   读时序：START → 器件地址(W) → 存储地址 → 重复 START → 器件地址(R)
 *           → 读数据… → STOP（即 bsp_i2c_read_reg 的复合时序）。
 * 注意：AT24C02 的存储地址是 8 位（0~255），页大小 8 字节；连续写不能跨页，
 * 一旦越过页边界会「回卷」到本页开头而不是写入下一页（见踩坑第 2 条）。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 器件地址 BSP_EEPROM_ADDR=0x50、容量 BSP_EEPROM_SIZE=256 均来自 board.h；
 * I2C0 总线引脚/频率来自 BSP_I2C0_*。本文件不写死地址。
 * 指南章节：第二十章 IIC_EEPROM实验。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * 1) 写完必须等待内部写周期，本文件在写函数内 vTaskDelay 10ms 兜底。
 * 2) 需保证「起始地址 + 长度」不跨 8 字节页边界，跨页多字节写行为不符合预期
 *    （示例里写 8 字节恰好一页，是安全的）。
 * 3) 存储地址以 (uint8_t) 截断传入，因此 addr 必须 ≤ 255（AT24C02 容量限制）。
 * 4) 写函数即使 base I2C 事务失败也照常延时后再返回错误码，不影响上层判断。
 */

#include "bsp_eeprom.h"
#include "bsp_i2c.h"
#include "board.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

/* 日志标签：本文件日志统一显示为 "bsp_eeprom" */
static const char *TAG = "bsp_eeprom";

/**
 * @brief 从指定存储地址写入一段数据
 *
 * 逻辑流程：
 *   ① 调用 I2C 原语：器件地址 + 存储地址(addr) → 数据 → STOP
 *   ② 延时 10ms 等待 EEPROM 内部写周期结束（期间器件不应答）
 *   ③ 若 I2C 事务失败则打印错误日志（延时已执行，不影响后续）
 *
 * @param addr 起始存储地址（0~255，AT24C02 内部字地址）
 * @param buf  待写入的数据缓冲区
 * @param len  待写入的字节数
 * @return ESP_OK 成功；其它为 I2C 错误码
 */
esp_err_t bsp_eeprom_write(uint16_t addr, const uint8_t *buf, uint16_t len)
{
    /* ① 把存储地址当作「寄存器地址」传入：形成 器件地址(W)+字地址+数据 的写时序。
     *    (uint8_t)addr 截断：AT24C02 字地址仅 8 位。 */
    esp_err_t ret = bsp_i2c_write_reg(BSP_EEPROM_ADDR, (uint8_t)addr, buf, len);
    vTaskDelay(pdMS_TO_TICKS(10));   /* ② 等待 EEPROM 内部写入周期 */
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "write failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

/**
 * @brief 从指定存储地址读出一段数据
 *
 * 逻辑流程：
 *   ① 调用 I2C 原语：写存储地址 → 重复起始 → 读数据（复合时序）
 *   ② 若读失败则打印错误日志
 *
 * @param addr 起始存储地址（0~255）
 * @param buf  接收缓冲区
 * @param len  待读取的字节数
 * @return ESP_OK 成功；其它为 I2C 错误码
 */
esp_err_t bsp_eeprom_read(uint16_t addr, uint8_t *buf, uint16_t len)
{
    /* ① 复合读时序：先写要访问的字地址，再从该地址起顺序读出 len 字节 */
    esp_err_t ret = bsp_i2c_read_reg(BSP_EEPROM_ADDR, (uint8_t)addr, buf, len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "read failed: %s", esp_err_to_name(ret));
    }
    return ret;
}
