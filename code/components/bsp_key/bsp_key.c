#include "bsp_key.h"
#include "board.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "bsp_key";

esp_err_t bsp_key_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BSP_KEY0_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,     /* 按键低电平有效，需要上拉 */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "KEY initialized on GPIO %d", BSP_KEY0_GPIO);
    return ESP_OK;
}

uint8_t bsp_key_scan(uint8_t mode)
{
    static uint8_t released = 1;   /* 1=已松开 */
    uint8_t keyval = 0;

    if (mode) {
        released = 1;              /* 支持连按 */
    }

    if (released && (gpio_get_level(BSP_KEY0_GPIO) == BSP_KEY0_ACTIVE_LEVEL)) {
        vTaskDelay(pdMS_TO_TICKS(10));   /* 软件消抖 */
        released = 0;
        if (gpio_get_level(BSP_KEY0_GPIO) == BSP_KEY0_ACTIVE_LEVEL) {
            keyval = 1;                  /* 确认按下 */
        }
    } else if (gpio_get_level(BSP_KEY0_GPIO) != BSP_KEY0_ACTIVE_LEVEL) {
        released = 1;                    /* 松开 */
    }

    return keyval;
}

/* ==================== 外部中断（EXIT） ==================== */

static volatile bool s_intr_triggered = false;

static void IRAM_ATTR key_isr_handler(void *arg)
{
    /* 中断里只做最少的事：置标志，具体处理交给任务 */
    s_intr_triggered = true;
}

esp_err_t bsp_key_intr_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BSP_KEY0_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = BSP_KEY0_INTR_TYPE,        /* 下降沿触发 */
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 安装 GPIO 中断服务（同一时刻只能安装一次） */
    ret = gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "gpio_install_isr_service failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = gpio_isr_handler_add(BSP_KEY0_GPIO, key_isr_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_isr_handler_add failed: %s", esp_err_to_name(ret));
        return ret;
    }

    gpio_intr_enable(BSP_KEY0_GPIO);
    ESP_LOGI(TAG, "KEY interrupt enabled on GPIO %d (falling edge)", BSP_KEY0_GPIO);
    return ESP_OK;
}

bool bsp_key_intr_triggered(void)
{
    if (s_intr_triggered) {
        s_intr_triggered = false;
        return true;
    }
    return false;
}
