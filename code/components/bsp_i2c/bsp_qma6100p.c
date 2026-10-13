/**
 * @file bsp_qma6100p.c
 * @brief QMA6100P 三轴加速度传感器驱动实现（I2C0，7 位地址 0x12）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 本文件通过 I2C0 驱动 QMA6100P 三轴加速度计：
 *   qma_write()             —— 内部：向某寄存器写一个字节
 *   bsp_qma6100p_init()     —— 复位 + 初始化序列 + 配置量程/带宽/电源 + 校验 CHIP_ID
 *   bsp_qma6100p_read_raw() —— 读回 X/Y/Z 三轴 14 位有符号原始值
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * 关键寄存器（寄存器地址见文件内宏定义）：
 *   0x00 CHIP_ID ：芯片身份寄存器，固定读回 0x90，用于确认通信正常。
 *   0x36 RESET   ：复位寄存器，先写 0xB6（软复位魔数）、再写 0x00 退出。
 *   0x0F RANGE   ：量程选择，写 0x04 = ±8G。
 *   0x10 BW_ODR  ：带宽/输出速率，写 0x00 = 带宽 100Hz。
 *   0x11 POWER   ：电源管理，写 0x84 = MCLK 51.2kHz | 激活。
 *   0x01~0x06    ：三轴数据 XOUTL/XOUTH、YOUTL/YOUTH、ZOUTL/ZOUTH。
 * 数据为 14 位有符号：先拼高 8 位、低 8 位成 16 位，再右移 2 位丢弃低 2 位
 * 无效位（读后 >>2 右对齐）。注意必须「先转成 int16_t 再算术右移」，
 * 这样负数才会按符号位正确扩展（若用无符号右移会丢掉符号）。
 * 静止平放时 Z 轴约等于 1G，由此可换算俯仰/翻滚角（见示例，与量程无关）。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 器件地址 BSP_QMA6100P_ADDR=0x12 来自 board.h；I2C0 引脚/频率来自 BSP_I2C0_*。
 * 本文件不写死地址。
 * 指南章节：第三十二章 IIC_QMA6100P实验。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * 1) 必须按「复位 → 初始化序列 → 配置 → 校验」的顺序执行，顺序错会导致读到错误数据。
 * 2) 复位魔数 0xB6 与初始化序列（0x11/0x4A/0x56/0x5F/0x21 等寄存器）来自
 *    QMA6100P 规格书 1a 节的推荐上电流程，不能随意更改或调换先后。
 * 3) 每一处写寄存器之间都要留出芯片内部建立时间（代码用短延时）。
 * 4) 初始化最后回读 CHIP_ID 校验，只有等于 0x90 才算初始化成功。
 */

#include "bsp_qma6100p.h"
#include "bsp_i2c.h"
#include "board.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

/* 日志标签：本文件日志统一显示为 "bsp_qma6100p" */
static const char *TAG = "bsp_qma6100p";

/* 寄存器地址 */
#define QMA_REG_CHIP_ID      0x00    /* 芯片 ID，固定 0x90 */
#define QMA_REG_XOUTL        0x01    /* X 轴输出低字节；三轴连续 6 字节（0x01~0x06） */
#define QMA_REG_RANGE        0x0F    /* 量程选择 */
#define QMA_REG_BW_ODR       0x10    /* 带宽 / 输出速率 */
#define QMA_REG_POWER        0x11    /* 电源管理 */
#define QMA_REG_RESET        0x36    /* 软复位寄存器 */

/**
 * @brief 内部辅助：向指定寄存器写一个字节
 *
 * @param reg 寄存器地址
 * @param val 待写入的值
 * @return ESP_OK 成功；其它为 I2C 错误码
 */
static esp_err_t qma_write(uint8_t reg, uint8_t val)
{
    /* 直接复用 I2C 写寄存器原语，把 &val 当作长度为 1 的数据 */
    return bsp_i2c_write_reg(BSP_QMA6100P_ADDR, reg, &val, 1);
}

/**
 * @brief 初始化 QMA6100P：复位 → 上电初始化序列 → 配置 → 校验 CHIP_ID
 *
 * 逻辑流程：
 *   ① 确保 I2C0 已初始化
 *   ② 软复位：写 0x36=0xB6，延时后写 0x36=0x00 退出复位
 *   ③ 按规格书 1a 节执行初始化序列（0x11/0x4A/0x56/0x5F 等）
 *   ④ 配置量程(8G)、带宽(100Hz)、电源(MCLK 51.2k|激活)
 *   ⑤ 回读 CHIP_ID，必须为 0x90，否则判定失败
 */
esp_err_t bsp_qma6100p_init(void)
{
    /* ① 确保 I2C0 总线就绪（内部幂等） */
    esp_err_t ret = bsp_i2c_init();
    if (ret != ESP_OK) return ret;

    /* ② 复位：0xB6 → 0x00。
     *    先写魔数 0xB6 触发软复位，等待复位完成后再写 0x00 让芯片恢复正常。 */
    qma_write(QMA_REG_RESET, 0xB6);
    vTaskDelay(pdMS_TO_TICKS(5));       /* 等复位动作生效 */
    qma_write(QMA_REG_RESET, 0x00);
    vTaskDelay(pdMS_TO_TICKS(10));      /* 等芯片稳定 */

    /* ③ 初始化序列（参考规格书 1a 节），顺序不可调换：
     *    0x11 电源管理先 0x80 后 0x84；0x4A/0x56/0x5F 为校准/配置寄存器；
     *    0x5F 先 0x80 再 0x00 以完成某个校准动作。 */
    qma_write(0x11, 0x80);
    qma_write(0x11, 0x84);
    qma_write(0x4A, 0x20);
    qma_write(0x56, 0x01);
    qma_write(0x5F, 0x80);
    vTaskDelay(pdMS_TO_TICKS(1));       /* 校准步之间留 1ms 建立时间 */
    qma_write(0x5F, 0x00);
    vTaskDelay(pdMS_TO_TICKS(10));      /* 等初始化序列完成 */

    qma_write(QMA_REG_RANGE, 0x04);     /* ④ 量程 8G */
    qma_write(QMA_REG_BW_ODR, 0x00);    /* 带宽 100Hz */
    qma_write(QMA_REG_POWER, 0x84);     /* MCLK 51.2K | 激活 */
    qma_write(0x21, 0x03);              /* 附加配置（规格书要求） */

    /* ⑤ 校验身份：CHIP_ID 固定应为 0x90 */
    uint8_t id = 0;
    bsp_i2c_read_reg(BSP_QMA6100P_ADDR, QMA_REG_CHIP_ID, &id, 1);
    if (id != 0x90) {
        ESP_LOGE(TAG, "CHIP_ID check fail (read 0x%02x)", id);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "QMA6100P init OK");
    return ESP_OK;
}

/**
 * @brief 读取 X/Y/Z 三轴原始加速度（14 位有符号）
 *
 * @param x 输出：X 轴原始值
 * @param y 输出：Y 轴原始值
 * @param z 输出：Z 轴原始值
 * @return ESP_OK 成功；其它为 I2C 错误码
 *
 * 逻辑流程：
 *   ① 从 XOUTL(0x01) 起连续读 6 字节（X/Y/Z 各 2 字节，低字节在前）
 *   ② 每轴按「高字节<<8 | 低字节」拼成 16 位，再右移 2 位对齐到 14 位有符号
 */
esp_err_t bsp_qma6100p_read_raw(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t buf[6] = {0};   /* 依次为 XOUTL/XOUTH/YOUTL/YOUTH/ZOUTL/ZOUTH */
    /* ① 连续读 6 字节，保证三轴来自同一次采样、互相同步 */
    esp_err_t ret = bsp_i2c_read_reg(BSP_QMA6100P_ADDR, QMA_REG_XOUTL, buf, 6);
    if (ret != ESP_OK) return ret;

    /* ② 先拼 16 位（高字节在后、低字节在前），转 int16 后再算术右移 2 位，
     *    取消低 2 位无效位并保持符号正确（负数正确扩展符号位）。 */
    *x = (int16_t)(((uint16_t)buf[1] << 8) | buf[0]) >> 2;
    *y = (int16_t)(((uint16_t)buf[3] << 8) | buf[2]) >> 2;
    *z = (int16_t)(((uint16_t)buf[5] << 8) | buf[4]) >> 2;
    return ESP_OK;
}
