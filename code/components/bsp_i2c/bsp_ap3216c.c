/**
 * @file bsp_ap3216c.c
 * @brief AP3216C 光强/接近/红外传感器驱动实现（I2C0，7 位地址 0x1E）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 本文件通过 I2C0 驱动 AP3216C 这颗三合一环境传感器：
 *   bsp_ap3216c_init() —— 软复位芯片、使能三路传感器并回读校验
 *   bsp_ap3216c_read() —— 一次性读回红外(IR)、环境光(ALS)、接近(PS) 三路数值
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * AP3216C 集成 IR（红外反射）、ALS（环境光强）、PS（接近）三路传感器，
 * 内部寄存器（本文件用到的）：
 *   系统配置寄存器 0x00：写 0x04 = 软件复位；写 0x03 = 使能 ALS + PS + IR。
 *   数据寄存器 0x0A~0x0F 连续 6 字节，依次为
 *     IR_L(0x0A)、IR_H(0x0B)、ALS_L(0x0C)、ALS_H(0x0D)、PS_L(0x0E)、PS_H(0x0F)。
 *   三路数据的拼位规则：
 *     IR（10 位）：buf[0] 的 bit7 为 IR_OF 溢出标志；低 2 位在 buf[0] 的 bit1:0，
 *                  高 8 位在 buf[1] → IR = (buf[1]<<2) | (buf[0]&0x03)。
 *     ALS（16 位）：高字节 buf[3]、低字节 buf[2] → ALS = (buf[3]<<8) | buf[2]。
 *     PS（10 位）：buf[4] 的 bit6 为 PS_OF 溢出标志；高 6 位在 buf[5] 的 bit5:0，
 *                  低 4 位在 buf[4] 的 bit3:0 → PS = ((buf[5]&0x3F)<<4) | (buf[4]&0x0F)。
 *   溢出位（OF）置 1 表示对应通道量程溢出、数据无效，代码把它当作 0 处理。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 器件地址 BSP_AP3216C_ADDR=0x1E 来自 board.h；I2C0 引脚/频率来自 BSP_I2C0_*。
 * 本文件不写死地址。
 * 指南章节：第二十五章 IIC_AP3216C实验。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * 1) 必须先 bsp_ap3216c_init()（内部会先初始化 I2C0）再读取。
 * 2) 软复位后需等待（本文件延时 50ms，规格要求 ≥10ms）再配置并使能。
 * 3) 三路全开时内部一次转换约 100ms，故两次读数间隔需 > 112.5ms，
 *    否则读到的可能是尚未刷新的旧数据（示例里用 500ms，留足余量）。
 * 4) ALS 为 16 位无符号，无需溢出判断；IR/PS 需按上面的 OF 位判断有效性。
 */

#include "bsp_ap3216c.h"
#include "bsp_i2c.h"
#include "board.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

/* 日志标签：本文件日志统一显示为 "bsp_ap3216c" */
static const char *TAG = "bsp_ap3216c";

#define AP3216C_REG_SYSCONF   0x00   /* 系统配置寄存器 */
#define AP3216C_REG_DATA      0x0A   /* 数据寄存器基址（连续 6 字节） */

/**
 * @brief 初始化 AP3216C：软复位 → 使能三路传感器 → 回读校验
 *
 * 逻辑流程：
 *   ① 确保 I2C0 已初始化
 *   ② 向系统配置寄存器写 0x04 软复位，延时等待复位完成
 *   ③ 向系统配置寄存器写 0x03 使能 ALS + PS + IR
 *   ④ 回读系统配置寄存器，校验确实为 0x03，否则报错返回
 */
esp_err_t bsp_ap3216c_init(void)
{
    /* ① 确保 I2C0 总线就绪（内部幂等） */
    esp_err_t ret = bsp_i2c_init();
    if (ret != ESP_OK) return ret;

    uint8_t v = 0x04;   /* 软件复位 */
    bsp_i2c_write_reg(BSP_AP3216C_ADDR, AP3216C_REG_SYSCONF, &v, 1);
    vTaskDelay(pdMS_TO_TICKS(50));   /* ② 复位至少 10ms，这里延时 50ms 留余量 */

    v = 0x03;           /* ③ 使能 ALS + PS + IR（bit0=ALS、bit1=PS/IR） */
    bsp_i2c_write_reg(BSP_AP3216C_ADDR, AP3216C_REG_SYSCONF, &v, 1);

    /* ④ 回读配置寄存器做校验：写不进说明器件未应答或有问题 */
    uint8_t tmp = 0;
    bsp_i2c_read_reg(BSP_AP3216C_ADDR, AP3216C_REG_SYSCONF, &tmp, 1);
    if (tmp != 0x03) {
        ESP_LOGE(TAG, "check fail (read 0x%02x)", tmp);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "AP3216C init OK");
    return ESP_OK;
}

/**
 * @brief 一次性读回 IR/ALS/PS 三路数据（含溢出判定与拼位）
 *
 * @param ir  输出：红外通道值（10 位有效，溢出时置 0）
 * @param als 输出：环境光强（16 位）
 * @param ps  输出：接近通道值（10 位有效，溢出时置 0）
 * @return ESP_OK 成功；其它为 I2C 错误码
 *
 * 逻辑流程：
 *   ① 从数据寄存器基址 0x0A 起连续读 6 字节到 buf
 *   ② 解析 IR：判 IR_OF 溢出位，未溢出则拼成 10 位值
 *   ③ 解析 ALS：高低字节拼成 16 位值
 *   ④ 解析 PS：判 PS_OF 溢出位，未溢出则拼成 10 位值
 */
esp_err_t bsp_ap3216c_read(uint16_t *ir, uint16_t *als, uint16_t *ps)
{
    uint8_t buf[6] = {0};   /* 依次存放 IR_L/IR_H/ALS_L/ALS_H/PS_L/PS_H */
    /* ① 一次性连续读 6 字节，避免三次单独读之间的数据不同步 */
    esp_err_t ret = bsp_i2c_read_reg(BSP_AP3216C_ADDR, AP3216C_REG_DATA, buf, 6);
    if (ret != ESP_OK) return ret;

    /* ② IR：buf[0] 的 bit7 = IR_OF；置 1 表示溢出，数据无效 */
    if (buf[0] & 0x80) {
        *ir = 0;
    } else {
        /* 高 8 位 = buf[1]，低 2 位 = buf[0] 的 bit1:0，拼成 10 位 */
        *ir = ((uint16_t)buf[1] << 2) | (buf[0] & 0x03);
    }

    /* ③ ALS：16 位，高字节 buf[3]，低字节 buf[2] */
    *als = ((uint16_t)buf[3] << 8) | buf[2];

    /* ④ PS：buf[4] 的 bit6 = PS_OF；置 1 表示溢出，数据无效 */
    if (buf[4] & 0x40) {
        *ps = 0;
    } else {
        /* 高 6 位 = buf[5] 的 bit5:0，低 4 位 = buf[4] 的 bit3:0，拼成 10 位 */
        *ps = ((uint16_t)(buf[5] & 0x3F) << 4) | (buf[4] & 0x0F);
    }
    return ESP_OK;
}
