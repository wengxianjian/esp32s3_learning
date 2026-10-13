/**
 * @file bsp_es8388.c
 * @brief ES8388 音频编解码器板级驱动实现（I2C0 控制）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 本文件对外提供三个函数：
 *   bsp_es8388_init()            —— 初始化 I2C0 并按固定序列配置 ES8388 各寄存器
 *   bsp_es8388_set_hp_volume()   —— 设置耳机（HP）音量（0~33）
 *   bsp_es8388_set_spk_volume()  —— 设置喇叭（SPK）音量（0~33）
 * 另有一个内部辅助函数 es8388_write_reg()，封装「写单个寄存器」。
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * ES8388 是一颗立体声编解码（ADC + DAC）芯片：
 *   播放：I2S 数据 → ES8388 DAC → 混频器 → 耳机/喇叭功放输出
 *   录音：麦克风 → ES8388 ADC → I2S 数据（由 I2S RX 接收）
 * 本文件通过 I2C0（地址 BSP_ES8388_ADDR=0x10）逐个写寄存器完成上电与通路配置，
 * 关键寄存器：0x00 复位、0x02 DAC/ADC 电源、0x17 DAC 数据格式、
 * 0x2E/0x2F 耳机（左右）音量、0x30/0x31 喇叭（左右）音量。
 * 配置序列遵循芯片手册推荐的上电流程，中间穿插延时等待内部电路稳定。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 从设备地址来自 board.h 的 BSP_ES8388_ADDR；
 * 寄存器地址与取值来自 ES8388 数据手册参考配置，本文件按 16bit/44.1kHz 场景设定。
 * 指南章节：第四十一章 音乐播放器、第四十二章 录音机。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * 1. 编解码器上电需按「先复位、再配电源、再配数据格式、最后配音量/混频」的顺序；
 * 2. 复位与配置之间、以及配置末尾需延时（100ms），等内部电路稳定；
 * 3. 音量寄存器有效范围为 0~33，超过会被函数钳位到 33；
 * 4. 本驱动依赖 I2C0 总线（bsp_i2c），I2S 数据通路由 bsp_i2s 负责，二者配合使用。
 */

#include "bsp_es8388.h"
#include "bsp_i2c.h"
#include "board.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

/* 日志标签：打印 ES8388 相关日志时统一显示为 "bsp_es8388" */
static const char *TAG = "bsp_es8388";

/**
 * @brief 向 ES8388 的指定寄存器写入 1 字节
 *
 * @param reg  寄存器地址（ES8388 内部寄存器编号）
 * @param val  要写入的值
 * @return     ESP_OK 成功，其它为 I2C 传输错误码
 *
 * @note 内部调用 bsp_i2c_write_reg()，从地址为 BSP_ES8388_ADDR，
 *       数据长度固定 1 字节（ES8388 寄存器均为 8 位）。
 */
static esp_err_t es8388_write_reg(uint8_t reg, uint8_t val)
{
    /* 向 7 位地址 BSP_ES8388_ADDR 的寄存器 reg 写 1 字节 val */
    return bsp_i2c_write_reg(BSP_ES8388_ADDR, reg, &val, 1);
}

/**
 * @brief 初始化 ES8388：初始化 I2C0 并写入完整配置序列
 *
 * 逻辑流程：
 *   ① 初始化 I2C0 主机总线（bsp_i2c_init）
 *   ② 软复位 ES8388，写完复位位后延时等待
 *   ③ 依次配置控制、电源、ADC/DAC 数据格式、时钟分频、音量、混频器
 *   ④ 打印初始化成功日志
 *
 * @return
 *      - ESP_OK  初始化成功
 *      - 其它    I2C 初始化失败，具体错误码见串口日志
 *
 * @note 寄存器写入本身未逐条检查返回值（教学演示从简）；
 *       若 ES8388 未接入或地址不对，串口会出现 I2C 错误。
 */
esp_err_t bsp_es8388_init(void)
{
    /* ── ① 初始化 I2C0 总线 ───────────────────────────────────
     * ES8388 的寄存器通过 I2C0 读写，故先初始化 I2C。 */
    esp_err_t ret = bsp_i2c_init();
    if (ret != ESP_OK) return ret;

    /* ── ② 软复位 ─────────────────────────────────────────────
     * 0x00 为控制寄存器（复位/参考电压等）。
     * 先写 0x80 触发复位，再写 0x00 清除复位位，随后延时等芯片稳定。 */
    es8388_write_reg(0x00, 0x80);   /* 复位 */
    es8388_write_reg(0x00, 0x00);
    vTaskDelay(pdMS_TO_TICKS(100));

    /* ── ③ 配置序列（寄存器地址 / 取值含义见行尾注释）────── */
    es8388_write_reg(0x01, 0x58);   /* 控制1 */
    es8388_write_reg(0x01, 0x50);
    es8388_write_reg(0x02, 0xF3);   /* DAC/ADC 电源管理：上电 */
    es8388_write_reg(0x02, 0xF0);
    es8388_write_reg(0x03, 0x09);   /* 麦克风偏置 */
    es8388_write_reg(0x00, 0x06);   /* 参考电压 */
    es8388_write_reg(0x04, 0x00);   /* DAC 电源管理 */
    es8388_write_reg(0x08, 0x00);   /* MCLK 分频 */
    es8388_write_reg(0x2B, 0x80);   /* DAC LRCK/ADC LRCK */
    es8388_write_reg(0x09, 0x88);   /* ADC PGA +24dB */
    es8388_write_reg(0x0C, 0x4C);   /* ADC 数据 16bit */
    es8388_write_reg(0x0D, 0x02);   /* ADC MCLK/rate=256 */
    es8388_write_reg(0x10, 0x00);   /* ADC 音量 L */
    es8388_write_reg(0x11, 0x00);   /* ADC 音量 R */
    es8388_write_reg(0x17, 0x18);   /* DAC 数据 16bit */
    es8388_write_reg(0x18, 0x02);   /* DAC MCLK/rate=256 */
    es8388_write_reg(0x1A, 0x00);   /* DAC 音量 L */
    es8388_write_reg(0x1B, 0x00);   /* DAC 音量 R */
    es8388_write_reg(0x27, 0xB8);   /* L 混频器 */
    es8388_write_reg(0x2A, 0xB8);   /* R 混频器 */
    vTaskDelay(pdMS_TO_TICKS(100)); /* 延时等内部通路稳定 */

    /* ── ④ 打印成功日志（地址以 2 位十六进制显示，便于核对硬件）── */
    ESP_LOGI(TAG, "ES8388 init OK (addr 0x%02x)", BSP_ES8388_ADDR);
    return ESP_OK;
}

/**
 * @brief 设置耳机（Headphone）音量
 *
 * @param volume 目标音量，有效范围 0~33，超出则按 33 处理
 *
 * @note 0x2E 为左声道耳机音量、0x2F 为右声道耳机音量，两声道写成同一值。
 */
void bsp_es8388_set_hp_volume(uint8_t volume)
{
    /* 越界保护：寄存器仅支持 0~33 档，大于 33 一律按最大档处理 */
    if (volume > 33) volume = 33;
    es8388_write_reg(0x2E, volume);
    es8388_write_reg(0x2F, volume);
}

/**
 * @brief 设置喇叭（Speaker）音量
 *
 * @param volume 目标音量，有效范围 0~33，超出则按 33 处理
 *
 * @note 0x30 为左声道喇叭音量、0x31 为右声道喇叭音量，两声道写成同一值。
 */
void bsp_es8388_set_spk_volume(uint8_t volume)
{
    /* 越界保护：寄存器仅支持 0~33 档，大于 33 一律按最大档处理 */
    if (volume > 33) volume = 33;
    es8388_write_reg(0x30, volume);
    es8388_write_reg(0x31, volume);
}
