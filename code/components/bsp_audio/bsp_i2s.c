/**
 * @file bsp_i2s.c
 * @brief I2S 音频接口板级驱动实现（I2S0，主模式）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 本文件对外提供两个函数：
 *   bsp_i2s_init()  —— 安装 I2S0 驱动（主模式、16bit、44100Hz 立体声）并绑定引脚
 *   bsp_i2s_write() —— 把一段 PCM 音频数据写入 I2S 发送缓冲（阻塞至写完）
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * ESP32-S3 内置 I2S 外设，这里把 I2S0 配成「主模式（Master）」，由它产生
 * 位时钟 BCK 与声道时钟 WS，ES8388 作为从设备跟随：
 *   播放：APP → bsp_i2s_write() → I2S TX FIFO/DMA → DO 引脚 → ES8388 DAC → 耳机/喇叭
 *   录音：ES8388 ADC → DI 引脚 → I2S RX FIFO/DMA（本驱动已使能 RX，供录音实验复用）
 * DMA：dma_buf_count=8 个缓冲区 × dma_buf_len=256 采样，
 *      用「双缓冲/多缓冲」边播边填，避免欠载造成爆音。
 * 音频格式：标准 I2S（I2S_COMM_FORMAT_STAND_I2S）、16bit、左右声道（RIGHT_LEFT）。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 采样率来自 board.h 的 BSP_AUDIO_SAMPLE_RATE；
 * BCK/WS/DO/DI/MCLK 引脚来自 BSP_I2S_* 宏，本文件不写死任何数字。
 * 指南章节：第四十一章 音乐播放器、第四十二章 录音机。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * 1. I2S 只负责「数据搬运」，音量、通路选择由 ES8388（I2C0）配置，二者需配合；
 * 2. MCLK 由 GPIO 输出（BSP_I2S_MCLK_GPIO），ES8388 需要主时钟才能工作；
 * 3. 本驱动 use_apll=false，采样率由内置 PLL 分频得到，44100Hz 下可正常工作；
 * 4. 初始化后先清零 DMA 缓冲，避免上电瞬间把残留数据当音频播出形成噪声。
 */

#include "bsp_i2s.h"
#include "board.h"
#include "driver/i2s.h"
#include "esp_log.h"

/* 日志标签：打印 I2S 相关日志时统一显示为 "bsp_i2s" */
static const char *TAG = "bsp_i2s";

/**
 * @brief 初始化 I2S0（主模式，16bit，44100Hz 立体声）并绑定引脚
 *
 * 逻辑流程：
 *   ① 用 i2s_driver_install() 按配置安装 I2S0 驱动（含 DMA）
 *   ② 用 i2s_set_pin() 把 I2S0 信号映射到 BCK/WS/DO/DI/MCLK 引脚
 *   ③ i2s_zero_dma_buffer() 清零 DMA 缓冲，避免残留数据造成噪声
 *
 * @return
 *      - ESP_OK   初始化成功
 *      - 其它     安装驱动或设置引脚失败，具体错误码见串口日志
 */
esp_err_t bsp_i2s_init(void)
{
    /* ── ① 安装 I2S 驱动 ──────────────────────────────────────
     * mode：主模式 + 同时使能发送（TX）与接收（RX），既可播放也可录音。
     * sample_rate：采样率，来自 BSP_AUDIO_SAMPLE_RATE（44100Hz）。
     * bits_per_sample：每个采样 16bit。
     * channel_format：RIGHT_LEFT，即立体声（左右两声道各 16bit 交替）。
     * communication_format：标准 I2S 时序（WS 低=左、高=右，数据相对 BCK 有一拍延迟）。
     * intr_alloc_flags=0：不绑定特定中断优先级（用默认）。
     * dma_buf_count=8、dma_buf_len=256：8 个 DMA 缓冲，每个 256 个采样，用于平滑数据流。
     * use_apll=false：不使用音频专用 APLL，由主 PLL 分频产生采样率。 */
    i2s_config_t cfg = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX,
        .sample_rate = BSP_AUDIO_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
    };
    /* 第 3、4 个参数为队列大小与队列句柄，这里不需要中断事件队列故传 0/NULL */
    esp_err_t ret = i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ── ② 绑定引脚 ────────────────────────────────────────────
     * bck_io_num：位时钟；ws_io_num：声道时钟（LRCLK）；
     * data_out_num：发送数据（接 ES8388 DAC）；data_in_num：接收数据（接 ES8388 ADC）；
     * mck_io_num：主时钟，供 ES8388 使用。全部来自 board.h 的 BSP_I2S_* 宏。 */
    i2s_pin_config_t pin = {
        .bck_io_num = BSP_I2S_BCK_GPIO,
        .ws_io_num = BSP_I2S_WS_GPIO,
        .data_out_num = BSP_I2S_DO_GPIO,
        .data_in_num = BSP_I2S_DI_GPIO,
        .mck_io_num = BSP_I2S_MCLK_GPIO,
    };
    ret = i2s_set_pin(I2S_NUM_0, &pin);
    if (ret != ESP_OK) return ret;

    /* ── ③ 清零 DMA 缓冲 ──────────────────────────────────────
     * 把缓冲内容清 0，避免上电时把随机残留数据当音频播出产生噪声。 */
    i2s_zero_dma_buffer(I2S_NUM_0);
    ESP_LOGI(TAG, "I2S0 init OK (%d Hz)", BSP_AUDIO_SAMPLE_RATE);
    return ESP_OK;
}

/**
 * @brief 把一段音频数据写入 I2S0 发送缓冲
 *
 * 逻辑流程：
 *   ① 调用 i2s_write() 阻塞式写入，直到 len 字节全部搬入 DMA
 *   ② 返回实际写入字节数
 *
 * @param buf  待发送的 PCM 数据首地址（16bit 立体声，即 L/R 交替）
 * @param len  待发送字节数
 * @return     实际写入的字节数
 *
 * @note portMAX_DELAY：缓冲满时一直等待直到有空位，保证数据不丢（阻塞调用）。
 */
size_t bsp_i2s_write(const uint8_t *buf, size_t len)
{
    /* written 用于接收实际写入字节数，初值 0 */
    size_t written = 0;
    /* 阻塞写入 I2S0，直到 len 字节全部写入或出错 */
    i2s_write(I2S_NUM_0, buf, len, &written, portMAX_DELAY);
    return written;
}
