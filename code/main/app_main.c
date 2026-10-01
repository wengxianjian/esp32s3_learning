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

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "DNESP32S3 learning project start");

#if defined(CONFIG_DEMO_LED)
    ESP_LOGI(TAG, "Running demo: LED");
    bsp_led_demo();
#elif defined(CONFIG_DEMO_KEY)
    ESP_LOGI(TAG, "Running demo: KEY");
    bsp_key_demo();
#elif defined(CONFIG_DEMO_EXIT)
    ESP_LOGI(TAG, "Running demo: EXIT");
    bsp_exit_demo();
#elif defined(CONFIG_DEMO_UART)
    ESP_LOGI(TAG, "Running demo: UART");
    bsp_uart_demo();
#elif defined(CONFIG_DEMO_ESPTIMER)
    ESP_LOGI(TAG, "Running demo: ESPTimer");
    bsp_esptimer_demo();
#elif defined(CONFIG_DEMO_GPTIMER)
    ESP_LOGI(TAG, "Running demo: GPTimer");
    bsp_gptimer_demo();
#elif defined(CONFIG_DEMO_WDT)
    ESP_LOGI(TAG, "Running demo: Watchdog");
    bsp_wdt_demo();
#elif defined(CONFIG_DEMO_SWPWM)
    ESP_LOGI(TAG, "Running demo: SW PWM");
    bsp_swpwm_demo();
#elif defined(CONFIG_DEMO_HWPWM)
    ESP_LOGI(TAG, "Running demo: HW PWM");
    bsp_hwpwm_demo();
#elif defined(CONFIG_DEMO_ADC)
    ESP_LOGI(TAG, "Running demo: ADC");
    bsp_adc_demo();
#elif defined(CONFIG_DEMO_RTC)
    ESP_LOGI(TAG, "Running demo: RTC");
    bsp_rtc_demo();
#elif defined(CONFIG_DEMO_RNG)
    ESP_LOGI(TAG, "Running demo: RNG");
    bsp_rng_demo();
#elif defined(CONFIG_DEMO_TSENS)
    ESP_LOGI(TAG, "Running demo: TSENS");
    bsp_tsens_demo();
#elif defined(CONFIG_DEMO_XL9555)
    ESP_LOGI(TAG, "Running demo: XL9555");
    bsp_xl9555_demo();
#elif defined(CONFIG_DEMO_EEPROM)
    ESP_LOGI(TAG, "Running demo: EEPROM");
    bsp_eeprom_demo();
#elif defined(CONFIG_DEMO_OLED)
    ESP_LOGI(TAG, "Running demo: OLED");
    bsp_oled_demo();
#elif defined(CONFIG_DEMO_AP3216C)
    ESP_LOGI(TAG, "Running demo: AP3216C");
    bsp_ap3216c_demo();
#elif defined(CONFIG_DEMO_QMA6100P)
    ESP_LOGI(TAG, "Running demo: QMA6100P");
    bsp_qma6100p_demo();
#elif defined(CONFIG_DEMO_LCD)
    ESP_LOGI(TAG, "Running demo: LCD");
    bsp_lcd_demo();
#elif defined(CONFIG_DEMO_DS18B20)
    ESP_LOGI(TAG, "Running demo: DS18B20");
    bsp_ds18b20_demo();
#elif defined(CONFIG_DEMO_DHT11)
    ESP_LOGI(TAG, "Running demo: DHT11");
    bsp_dht11_demo();
#elif defined(CONFIG_DEMO_IR_RX)
    ESP_LOGI(TAG, "Running demo: IR RX");
    bsp_ir_rx_demo();
#elif defined(CONFIG_DEMO_IR_TX)
    ESP_LOGI(TAG, "Running demo: IR TX");
    bsp_ir_tx_demo();
#elif defined(CONFIG_DEMO_SDCARD)
    ESP_LOGI(TAG, "Running demo: SD card");
    bsp_sdcard_demo();
#elif defined(CONFIG_DEMO_SPIFFS)
    ESP_LOGI(TAG, "Running demo: SPIFFS");
    bsp_spiffs_demo();
#elif defined(CONFIG_DEMO_AUDIO)
    ESP_LOGI(TAG, "Running demo: Audio");
    bsp_audio_demo();
#elif defined(CONFIG_DEMO_USB)
    ESP_LOGI(TAG, "Running demo: USB MSC");
    bsp_usb_msc_demo();
#elif defined(CONFIG_DEMO_WIFI_SCAN)
    ESP_LOGI(TAG, "Running demo: WiFi scan");
    bsp_wifi_scan_demo();
#elif defined(CONFIG_DEMO_WIFI_STA)
    ESP_LOGI(TAG, "Running demo: WiFi STA");
    bsp_wifi_sta_demo();
#elif defined(CONFIG_DEMO_WIFI_AP)
    ESP_LOGI(TAG, "Running demo: WiFi AP");
    bsp_wifi_ap_demo();
#elif defined(CONFIG_DEMO_WIFI_SMARTCONFIG)
    ESP_LOGI(TAG, "Running demo: WiFi SmartConfig");
    bsp_wifi_smartconfig_demo();
#elif defined(CONFIG_DEMO_UDP)
    ESP_LOGI(TAG, "Running demo: UDP");
    bsp_udp_demo();
#elif defined(CONFIG_DEMO_TCP_CLIENT)
    ESP_LOGI(TAG, "Running demo: TCP client");
    bsp_tcp_client_demo();
#elif defined(CONFIG_DEMO_TCP_SERVER)
    ESP_LOGI(TAG, "Running demo: TCP server");
    bsp_tcp_server_demo();
#elif defined(CONFIG_DEMO_MQTT)
    ESP_LOGI(TAG, "Running demo: MQTT");
    bsp_mqtt_demo();
#else
    ESP_LOGE(TAG, "No demo selected! Run 'idf.py menuconfig' to pick a demo.");
#endif
}
