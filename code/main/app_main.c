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
#else
    ESP_LOGE(TAG, "No demo selected! Run 'idf.py menuconfig' to pick a demo.");
#endif
}
