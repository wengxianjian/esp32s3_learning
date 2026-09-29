#include "bsp_led.h"
#include "board.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "bsp_led";

esp_err_t bsp_led_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BSP_LED0_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 初始熄灭 */
    bsp_led_set(false);
    ESP_LOGI(TAG, "LED initialized on GPIO %d", BSP_LED0_GPIO);
    return ESP_OK;
}

esp_err_t bsp_led_set(bool on)
{
    /* 根据有效电平换算实际 GPIO 电平 */
    uint32_t level = (BSP_LED0_ACTIVE_LEVEL == 0) ? (on ? 0 : 1) : (on ? 1 : 0);
    return gpio_set_level(BSP_LED0_GPIO, level);
}

esp_err_t bsp_led_toggle(void)
{
    return gpio_set_level(BSP_LED0_GPIO, !gpio_get_level(BSP_LED0_GPIO));
}
