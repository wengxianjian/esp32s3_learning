/**
 * @file qma6100p_demo.c
 * @brief QMA6100P 三轴加速度实验示例（对应指南第三十二章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 初始化 QMA6100P（复位 + 初始化序列 + 配置 + 校验 CHIP_ID）；
 * 2) 进入死循环，每 300ms 读一次 X/Y/Z 原始值，并换算俯仰(pitch)/翻滚(roll)角打印；
 * 3) 平放时 Z≈1G、X/Y≈0；倾斜开发板可看到角度与三轴数值随之变化。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * menuconfig 选 "QMA6100P"（宏 CONFIG_DEMO_QMA6100P），
 * app_main() 调用 bsp_qma6100p_demo()。
 */

#include "bsp_qma6100p.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

/* 日志标签：本示例日志显示为 "qma6100p_demo" */
static const char *TAG = "qma6100p_demo";

/**
 * @brief QMA6100P 实验入口：周期读取三轴并计算俯仰/翻滚角
 *
 * 逻辑流程：
 *   ① 初始化 QMA6100P（失败断言复位）
 *   ② 死循环：每 300ms 读三轴原始值，用 atan2 计算 pitch/roll 并打印
 */
void bsp_qma6100p_demo(void)
{
    ESP_LOGI(TAG, "QMA6100P demo start");
    /* 初始化传感器（内部会先初始化 I2C0） */
    ESP_ERROR_CHECK(bsp_qma6100p_init());

    int16_t x, y, z;    /* 三轴原始加速度（14 位有符号，静止时某一轴约为 1G 重力对应的读数） */
    while (1) {
        /* 读三轴原始值；失败会断言复位 */
        ESP_ERROR_CHECK(bsp_qma6100p_read_raw(&x, &y, &z));
        /* 由重力在三轴上的分量计算倾角：
         *   pitch = atan2(X, sqrt(Y²+Z²))
         *   roll  = atan2(Y, sqrt(X²+Z²))
         * atan2 返回弧度，乘 180/π 转成角度；该比值为比例关系、与当前量程无关。 */
        float pitch = atan2f(x, sqrtf((float)y * y + (float)z * z)) * 180.0f / M_PI;
        float roll  = atan2f(y, sqrtf((float)x * x + (float)z * z)) * 180.0f / M_PI;
        ESP_LOGI(TAG, "X=%d Y=%d Z=%d | pitch=%.1f roll=%.1f", x, y, z, pitch, roll);
        vTaskDelay(pdMS_TO_TICKS(300));     /* 每 300ms 读一次 */
    }
}
