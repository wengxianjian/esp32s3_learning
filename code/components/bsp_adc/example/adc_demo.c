/**
 * @file adc_demo.c
 * @brief ADC 实验示例（对应指南第二十四章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 先初始化 ADC1 通道 7（GPIO8）；
 * 2) 然后进入死循环，每 1 秒读一次原始值 raw 和电压 mV，通过串口打印；
 * 3) 旋转板上电位器（需用跳线帽接到 ADC_IN），即可看到数值跟着变化。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "ADC" 这个 demo（对应宏 CONFIG_DEMO_ADC），
 * app_main() 就会调用 bsp_adc_demo()。
 */

#include "bsp_adc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 日志标签：本示例打印的日志会显示为 "adc_demo" */
static const char *TAG = "adc_demo";

void bsp_adc_demo(void)
{
    /* 提示开始：注意这里写死了一句说明用的是 ADC1_CHANNEL_7 (GPIO8)，
     * 便于对照硬件；实际通道号以 board.h 的 BSP_ADC_CHANNEL 为准。 */
    ESP_LOGI(TAG, "ADC demo start: read ADC1_CHANNEL_7 (GPIO8)");

    /* 初始化 ADC。ESP_ERROR_CHECK：若初始化失败（返回非 ESP_OK），
     * 会直接打印断言信息并复位，避免带病运行。 */
    ESP_ERROR_CHECK(bsp_adc_init());

    while (1) {
        /* 采集一次原始值（12 位，0~4095） */
        int raw = bsp_adc_read_raw();
        /* 采集一次并换算为电压（mV）。注意内部会再采集一次，因此 raw 与 mv
         * 来自两次相邻采样，数值上可能有一点点差异，属正常现象。 */
        uint32_t mv = bsp_adc_read_mv();
        /* 打印结果：raw 用 %d，mv 用无符号长整型 %lu（显式转 unsigned long 以兼容） */
        ESP_LOGI(TAG, "raw = %d, voltage = %lu mV", raw, (unsigned long)mv);
        /* 每 1 秒采集打印一次（1000 个 tick；默认 1 tick = 1ms） */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
