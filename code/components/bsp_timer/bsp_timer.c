#include "bsp_timer.h"
#include "board.h"
#include "bsp_led.h"
#include "esp_timer.h"
#include "driver/gptimer.h"
#include "esp_log.h"

static const char *TAG = "bsp_timer";
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

/* ==================== GPTimer 硬件定时器 ==================== */

static gptimer_handle_t s_gptimer = NULL;
static volatile uint32_t s_gptimer_tick = 0;

static bool IRAM_ATTR gptimer_alarm_cb(gptimer_handle_t timer,
                                       const gptimer_alarm_event_data_t *edata,
                                       void *user_ctx)
{
    (void)timer; (void)edata; (void)user_ctx;
    s_gptimer_tick++;   /* 仅计数，ISR 内不做过重操作 */
    return false;       /* 已在 ISR 内处理，无需唤醒高优先级任务 */
}

esp_err_t bsp_gptimer_init(void)
{
    gptimer_config_t cfg = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,   /* 1MHz -> 1 tick = 1us */
    };
    esp_err_t ret = gptimer_new_timer(&cfg, &s_gptimer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gptimer_new_timer failed: %s", esp_err_to_name(ret));
        return ret;
    }

    gptimer_alarm_config_t alarm = {
        .alarm_count = BSP_GPTIMER_PERIOD_US,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    ret = gptimer_set_alarm_action(s_gptimer, &alarm);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gptimer_set_alarm_action failed: %s", esp_err_to_name(ret));
        return ret;
    }

    gptimer_event_callbacks_t cbs = {
        .on_alarm = gptimer_alarm_cb,
    };
    ret = gptimer_register_event_callbacks(s_gptimer, &cbs, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gptimer_register_event_callbacks failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = gptimer_enable(s_gptimer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gptimer_enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = gptimer_start(s_gptimer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gptimer_start failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "GPTimer started, period = %d ms", BSP_GPTIMER_PERIOD_US / 1000);
    return ESP_OK;
}

uint32_t bsp_gptimer_get_tick_count(void)
{
    return s_gptimer_tick;
}
