#include "bsp_timer.h"
#include "board.h"
#include "bsp_led.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "bsp_esptimer";
static esp_timer_handle_t s_timer = NULL;
static volatile uint32_t s_tick_count = 0;

static void esp_timer_cb(void *arg)
{
    (void)arg;
    s_tick_count++;
    bsp_led_toggle();
}

esp_err_t bsp_esptimer_init(void)
{
    esp_timer_create_args_t args = {
        .callback = esp_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,   /* 在 esp_timer 任务上下文执行回调 */
        .name = "led_timer",
    };

    esp_err_t ret = esp_timer_create(&args, &s_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_timer_create failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_timer_start_periodic(s_timer, BSP_ESPTIMER_PERIOD_US);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_timer_start_periodic failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "ESPTimer started, period = %d ms", BSP_ESPTIMER_PERIOD_US / 1000);
    return ESP_OK;
}

uint32_t bsp_esptimer_get_tick_count(void)
{
    return s_tick_count;
}
