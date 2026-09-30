#include "esp_log.h"
#include "bsp_led.h"
#include "bsp_key.h"

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
#else
    ESP_LOGE(TAG, "No demo selected! Run 'idf.py menuconfig' to pick a demo.");
#endif
}
