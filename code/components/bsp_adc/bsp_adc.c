/**
 * @file bsp_adc.c
 * @brief ADC 板级驱动实现（ADC1 通道 7 = GPIO8）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 封装 ESP-IDF 的 ADC「oneshot（单次采集）」驱动，对外提供三个函数：
 *   bsp_adc_init()      —— 创建 ADC 单元 + 配置通道（衰减、位宽）
 *   bsp_adc_read_raw()  —— 采集一次，返回 12 位原始值
 *   bsp_adc_read_mv()   —— 采集一次并以 mV 返回（线性近似换算）
 *
 * ── 数据流 ───────────────────────────────────────────────────
 *   GPIO8 上的模拟电压
 *      → SAR ADC 采样量化
 *      → 得到 0~4095 的原始值（raw）
 *      → 乘 3100/4095 换算成 mV
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 单元/通道/衰减/位宽均来自 board.h 的 BSP_ADC_* 宏，本文件不写死任何数字。
 * 指南章节：第二十四章 ADC实验。
 *
 * ── ESP-IDF 新版 API 说明 ────────────────────────────────────
 * 使用 esp_adc/adc_oneshot.h（oneshot 单次采集模式）；
 * 旧的 adc1_get_raw() 已废弃，故不再使用。
 */

#include "bsp_adc.h"
#include "board.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

/* 日志标签：打印 ADC 相关日志时统一显示为 "bsp_adc" */
static const char *TAG = "bsp_adc";

/* ADC 单元句柄：由 bsp_adc_init() 创建，后续读值都依赖它。
 * 用 static 保存，避免每次读值都要重新创建单元。初始为 NULL 表示尚未初始化。 */
static adc_oneshot_unit_handle_t s_adc = NULL;

/**
 * @brief 初始化 ADC：创建 ADC1 单元并配置通道（衰减 / 位宽）
 *
 * 逻辑流程：
 *   ① 用 BSP_ADC_UNIT 指定的单元号创建 ADC 单元句柄 s_adc
 *   ② 用 BSP_ADC_ATTEN / BSP_ADC_BITWIDTH 配置目标通道
 *   ③ 任一步失败则打印错误并立即返回
 */
esp_err_t bsp_adc_init(void)
{
    /* ── ① 创建 ADC 单元 ────────────────────────────────────────
     * unit_id：选择哪个 ADC 单元。ESP32-S3 有 ADC1、ADC2 两个 SAR ADC，
     *          这里由 BSP_ADC_UNIT 决定（本板用 ADC_UNIT_1）。
     *          ADC1 可配合 Wi-Fi 使用，ADC2 与 Wi-Fi 有冲突，故一般优先用 ADC1。 */
    adc_oneshot_unit_init_cfg_t cfg = {
        .unit_id = BSP_ADC_UNIT,
    };
    esp_err_t ret = adc_oneshot_new_unit(&cfg, &s_adc);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ── ② 配置通道参数 ────────────────────────────────────────
     * atten    ：衰减档位，决定量程。BSP_ADC_ATTEN = ADC_ATTEN_DB_11，
     *            输入量程约 0~3.1V（超过会被削顶，读数不再线性）。
     * bitwidth ：分辨率。BSP_ADC_BITWIDTH = ADC_BITWIDTH_12，
     *            即 12 位，原始值范围 0~4095。 */
    adc_oneshot_chan_cfg_t chan = {
        .atten = BSP_ADC_ATTEN,
        .bitwidth = BSP_ADC_BITWIDTH,
    };
    ret = adc_oneshot_config_channel(s_adc, BSP_ADC_CHANNEL, &chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_config_channel failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ── ③ 初始化完成，打印所用通道号（方便对照硬件确认引脚）────── */
    ESP_LOGI(TAG, "ADC1 channel %d initialized", BSP_ADC_CHANNEL);
    return ESP_OK;
}

/**
 * @brief 采集一次，返回 12 位原始值（0~4095）
 *
 * raw：接收采集结果的变量。adc_oneshot_read() 把转换结果写入 raw。
 * 这里未检查返回值（教学演示从简）；若通道未初始化会读失败，raw 保持 0。
 */
int bsp_adc_read_raw(void)
{
    int raw = 0;                                    /* 结果缓冲，初值 0 */
    adc_oneshot_read(s_adc, BSP_ADC_CHANNEL, &raw); /* 从指定通道读一次 */
    return raw;                                     /* 返回 0~4095 的数字量 */
}

/**
 * @brief 采集一次并换算成电压 mV
 *
 * 换算公式：mV = raw × 3100 / 4095
 *   3100 —— 11dB 衰减下的近似满量程（约 3.1V）
 *   4095 —— 12 位 ADC 最大值（2^12 - 1）
 * 先乘后除，避免整数除法把小数部分提前丢掉（提高一点精度）。
 */
uint32_t bsp_adc_read_mv(void)
{
    int raw = bsp_adc_read_raw();
    /* 11dB 衰减量程约 0~3100mV，12 位 0~4095，近似线性换算 */
    return (uint32_t)(raw * 3100 / 4095);
}
