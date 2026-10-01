# I2S 音频 + ES8388 学习笔记

## 硬件
- ES8388 音频编解码器，I2C0 控制（地址 0x10），I2S0 传数据。
- I2S：BCK=IO46、WS=IO9、DO=IO10、DI=IO14、MCLK=IO3。
- 音频模块插在板载音频接口。

## 原理要点
- I2S 主模式，16bit，44100Hz，标准 I2S 格式。
- ES8388 通过 I2C 配置（复位、电源、DAC/ADC 数据格式、音量、混频器）。
- 播放：I2S TX → ES8388 DAC → 耳机/喇叭；录音：ES8388 ADC → I2S RX。
- ES8388 关键寄存器：0x00 复位、0x02 DAC/ADC 电源、0x17 DAC 数据格式、0x2E/2F 耳机音量、0x30/31 喇叭音量。

## 宏设计
- `BSP_ES8388_ADDR`、`BSP_I2S_BCK/WS/DO/DI/MCLK_GPIO`、`BSP_AUDIO_SAMPLE_RATE`。

## 验证结果

- ✅ `ES8388 init OK (addr 0x10)` + `I2S0 init OK (44100 Hz)`
- ✅ 正在输出 1kHz 正弦音（插耳机可听到）
