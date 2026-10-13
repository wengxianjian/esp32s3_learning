/**
 * @file app_main.c
 * @brief 顶层入口：按 menuconfig 选择的 demo 分发到对应主循环
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 本文件是整个工程的唯一入口 app_main()。它自身不做任何硬件操作，而是根据
 * menuconfig 中选中的 demo（编译期生成的 CONFIG_DEMO_* 宏），用一串
 * #if / #elif 把执行分发到对应组件的 bsp_xxx_demo()；各 demo 函数内部通常是
 * 一个 while(1) 死循环，因此一次只运行一个实验。
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * menuconfig（选项定义在 code/main/Kconfig）
 *   → 生成的 sdkconfig 宏 CONFIG_DEMO_XXX
 *   → 编译期 #if defined(CONFIG_DEMO_XXX) 命中唯一分支
 *   → 调用 bsp_xxx_demo()，进入该实验的主循环。
 * 因为是"编译期分发"，切换实验需要重新 menuconfig 并重新编译烧录。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * ① 同一时刻只能选中一个 demo；若一个都没选，会走 #else 分支打印错误提示；
 * ② 这里包含所有组件的头文件只为分发调用方便，未被选中的 demo 不会被调用。
 */

/* 汇总包含所有组件的接口头文件：app_main 需要按选择调用各 bsp_xxx_demo() */
#include "esp_log.h"
#include "bsp_led.h"
#include "bsp_key.h"
#include "bsp_uart.h"
#include "bsp_timer.h"
#include "bsp_pwm.h"
#include "bsp_adc.h"
#include "bsp_rtc_rng.h"
#include "bsp_i2c.h"
#include "bsp_eeprom.h"
#include "bsp_oled.h"
#include "bsp_ap3216c.h"
#include "bsp_qma6100p.h"
#include "bsp_lcd.h"
#include "bsp_ds18b20.h"
#include "bsp_dht11.h"
#include "bsp_ir_rx.h"
#include "bsp_ir_tx.h"
#include "bsp_sdcard.h"
#include "bsp_spiffs.h"
#include "bsp_es8388.h"
#include "bsp_i2s.h"
#include "bsp_usb.h"
#include "bsp_wifi.h"
#include "bsp_net.h"
#include "bsp_mqtt.h"

/* 日志标签：顶层入口打印的日志会显示为 "main" */
static const char *TAG = "main";

/**
 * @brief 工程入口：读取 menuconfig 选择，分发到唯一的 demo 主循环
 *
 * 逻辑流程：
 *   ① 打印工程启动日志
 *   ② 用编译期 #if / #elif 判断选中的 CONFIG_DEMO_* 宏
 *   ③ 调用对应的 bsp_xxx_demo()（内部为 while(1)，正常情况下不会返回）
 */
void app_main(void)
{
    ESP_LOGI(TAG, "DNESP32S3 learning project start");

    /* ══ 以下为一串编译期分发：只会命中其中一个 #elif 分支，
     *    每个分支调用对应组件的 demo 主循环（进入后通常不再返回）。 ══ */
#if defined(CONFIG_DEMO_LED)
    /* 第 10 章 LED 实验：LED 定时闪烁 */
    ESP_LOGI(TAG, "Running demo: LED");
    bsp_led_demo();
#elif defined(CONFIG_DEMO_KEY)
    /* 第 11 章 KEY 实验：轮询按键翻转 LED */
    ESP_LOGI(TAG, "Running demo: KEY");
    bsp_key_demo();
#elif defined(CONFIG_DEMO_EXIT)
    /* EXIT 实验：外部中断方式检测按键翻转 LED */
    ESP_LOGI(TAG, "Running demo: EXIT");
    bsp_exit_demo();
#elif defined(CONFIG_DEMO_UART)
    /* 第 13 章 UART 实验：串口收发/回显 */
    ESP_LOGI(TAG, "Running demo: UART");
    bsp_uart_demo();
#elif defined(CONFIG_DEMO_ESPTIMER)
    /* 第 14 章 ESPTimer 软件定时器实验 */
    ESP_LOGI(TAG, "Running demo: ESPTimer");
    bsp_esptimer_demo();
#elif defined(CONFIG_DEMO_GPTIMER)
    /* 第 15 章 GPTimer 硬件定时器实验 */
    ESP_LOGI(TAG, "Running demo: GPTimer");
    bsp_gptimer_demo();
#elif defined(CONFIG_DEMO_WDT)
    /* 第 16 章 Watchdog 任务看门狗实验 */
    ESP_LOGI(TAG, "Running demo: Watchdog");
    bsp_wdt_demo();
#elif defined(CONFIG_DEMO_SWPWM)
    /* 第 17 章 软件 PWM 实验（LEDC 软件渐变） */
    ESP_LOGI(TAG, "Running demo: SW PWM");
    bsp_swpwm_demo();
#elif defined(CONFIG_DEMO_HWPWM)
    /* 第 18 章 硬件 PWM 实验（LEDC 硬件渐变） */
    ESP_LOGI(TAG, "Running demo: HW PWM");
    bsp_hwpwm_demo();
#elif defined(CONFIG_DEMO_ADC)
    /* 第 24 章 ADC 实验：读取模拟电压 */
    ESP_LOGI(TAG, "Running demo: ADC");
    bsp_adc_demo();
#elif defined(CONFIG_DEMO_RTC)
    /* RTC 实验：片上实时时钟 */
    ESP_LOGI(TAG, "Running demo: RTC");
    bsp_rtc_demo();
#elif defined(CONFIG_DEMO_RNG)
    /* RNG 实验：硬件随机数发生器 */
    ESP_LOGI(TAG, "Running demo: RNG");
    bsp_rng_demo();
#elif defined(CONFIG_DEMO_TSENS)
    /* 第 28 章 内部温度传感器实验 */
    ESP_LOGI(TAG, "Running demo: TSENS");
    bsp_tsens_demo();
#elif defined(CONFIG_DEMO_XL9555)
    /* 第 19 章 IIC_EXIO 实验：XL9555 IO 扩展（含蜂鸣器） */
    ESP_LOGI(TAG, "Running demo: XL9555");
    bsp_xl9555_demo();
#elif defined(CONFIG_DEMO_EEPROM)
    /* 第 20 章 IIC_EEPROM 实验：AT24C02 读写 */
    ESP_LOGI(TAG, "Running demo: EEPROM");
    bsp_eeprom_demo();
#elif defined(CONFIG_DEMO_OLED)
    /* 第 21 章 IIC_OLED 实验：SSD1306 显示 */
    ESP_LOGI(TAG, "Running demo: OLED");
    bsp_oled_demo();
#elif defined(CONFIG_DEMO_AP3216C)
    /* 第 25 章 IIC_AP3216C 实验：光强/接近传感器 */
    ESP_LOGI(TAG, "Running demo: AP3216C");
    bsp_ap3216c_demo();
#elif defined(CONFIG_DEMO_QMA6100P)
    /* 第 32 章 IIC_QMA6100P 实验：三轴加速度传感器 */
    ESP_LOGI(TAG, "Running demo: QMA6100P");
    bsp_qma6100p_demo();
#elif defined(CONFIG_DEMO_LCD)
    /* 第 22 章 SPI_LCD 实验：ILI9341 显示 */
    ESP_LOGI(TAG, "Running demo: LCD");
    bsp_lcd_demo();
#elif defined(CONFIG_DEMO_DS18B20)
    /* 第 29 章 单总线 DS18B20 温度实验 */
    ESP_LOGI(TAG, "Running demo: DS18B20");
    bsp_ds18b20_demo();
#elif defined(CONFIG_DEMO_DHT11)
    /* 第 30 章 单总线 DHT11 温湿度实验 */
    ESP_LOGI(TAG, "Running demo: DHT11");
    bsp_dht11_demo();
#elif defined(CONFIG_DEMO_IR_RX)
    /* 第 26 章 红外接收实验（RMT + NEC 解码） */
    ESP_LOGI(TAG, "Running demo: IR RX");
    bsp_ir_rx_demo();
#elif defined(CONFIG_DEMO_IR_TX)
    /* 第 27 章 红外发送实验（RMT + NEC 编码） */
    ESP_LOGI(TAG, "Running demo: IR TX");
    bsp_ir_tx_demo();
#elif defined(CONFIG_DEMO_SDCARD)
    /* 第 37 章 SPI_SDCARD 实验：挂载 TF 卡文件系统 */
    ESP_LOGI(TAG, "Running demo: SD card");
    bsp_sdcard_demo();
#elif defined(CONFIG_DEMO_SPIFFS)
    /* 第 38 章 SPIFFS 实验：内部 Flash 文件系统 */
    ESP_LOGI(TAG, "Running demo: SPIFFS");
    bsp_spiffs_demo();
#elif defined(CONFIG_DEMO_AUDIO)
    /* 第 41/42 章 音频实验：ES8388 + I2S 播放/录音 */
    ESP_LOGI(TAG, "Running demo: Audio");
    bsp_audio_demo();
#elif defined(CONFIG_DEMO_USB)
    /* USB MSC 实验：把设备模拟成 U 盘 */
    ESP_LOGI(TAG, "Running demo: USB MSC");
    bsp_usb_msc_demo();
#elif defined(CONFIG_DEMO_WIFI_SCAN)
    /* Wi-Fi 实验：扫描周边热点 */
    ESP_LOGI(TAG, "Running demo: WiFi scan");
    bsp_wifi_scan_demo();
#elif defined(CONFIG_DEMO_WIFI_STA)
    /* Wi-Fi 实验：Station 模式连接路由器 */
    ESP_LOGI(TAG, "Running demo: WiFi STA");
    bsp_wifi_sta_demo();
#elif defined(CONFIG_DEMO_WIFI_AP)
    /* Wi-Fi 实验：SoftAP 模式开启热点 */
    ESP_LOGI(TAG, "Running demo: WiFi AP");
    bsp_wifi_ap_demo();
#elif defined(CONFIG_DEMO_WIFI_SMARTCONFIG)
    /* Wi-Fi 实验：SmartConfig 一键配网 */
    ESP_LOGI(TAG, "Running demo: WiFi SmartConfig");
    bsp_wifi_smartconfig_demo();
#elif defined(CONFIG_DEMO_UDP)
    /* 网络实验：UDP 收发 */
    ESP_LOGI(TAG, "Running demo: UDP");
    bsp_udp_demo();
#elif defined(CONFIG_DEMO_TCP_CLIENT)
    /* 网络实验：TCP 客户端 */
    ESP_LOGI(TAG, "Running demo: TCP client");
    bsp_tcp_client_demo();
#elif defined(CONFIG_DEMO_TCP_SERVER)
    /* 网络实验：TCP 服务端 */
    ESP_LOGI(TAG, "Running demo: TCP server");
    bsp_tcp_server_demo();
#elif defined(CONFIG_DEMO_MQTT)
    /* 网络实验：MQTT 客户端（发布/订阅） */
    ESP_LOGI(TAG, "Running demo: MQTT");
    bsp_mqtt_demo();
#else
    /* 一个 demo 都没选：给出明确提示，避免上电后"毫无反应"而无从排查 */
    ESP_LOGE(TAG, "No demo selected! Run 'idf.py menuconfig' to pick a demo.");
#endif
}
