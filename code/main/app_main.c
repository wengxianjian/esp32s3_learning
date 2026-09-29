#include "esp_log.h"
#include "bsp_led.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "DNESP32S3 learning project start");

#ifdef CONFIG_DEMO_LED
    ESP_LOGI(TAG, "Running demo: LED");
    bsp_led_demo();
#else
    ESP_LOGE(TAG, "No demo selected! Run 'idf.py menuconfig' to pick a demo.");
#endif
}
