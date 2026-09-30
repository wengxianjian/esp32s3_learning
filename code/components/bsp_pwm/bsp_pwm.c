#include "bsp_pwm.h"
#include "board.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "bsp_pwm";

esp_err_t bsp_pwm_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = BSP_PWM_LEDC_TIMER,
        .duty_resolution = BSP_PWM_DUTY_RES,
        .freq_hz = BSP_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t ret = ledc_timer_config(&timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc_timer_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ledc_channel_config_t ch = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = BSP_PWM_LEDC_CHANNEL,
        .timer_sel = BSP_PWM_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = BSP_LED0_GPIO,
        .duty = 0,
        .hpoint = 0,
    };
    ret = ledc_channel_config(&ch);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc_channel_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "LEDC PWM init: %d Hz, GPIO %d", BSP_PWM_FREQ_HZ, BSP_LED0_GPIO);
    return ESP_OK;
}

esp_err_t bsp_pwm_set_duty(uint32_t duty)
{
    esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, BSP_PWM_LEDC_CHANNEL, duty);
    if (ret != ESP_OK) {
        return ret;
    }
    return ledc_update_duty(LEDC_LOW_SPEED_MODE, BSP_PWM_LEDC_CHANNEL);
}

esp_err_t bsp_pwm_fade_to(uint32_t target_duty, int time_ms)
{
    /* 安装渐变功能（重复安装返回错误，忽略） */
    ledc_fade_func_install(0);

    esp_err_t ret = ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, BSP_PWM_LEDC_CHANNEL,
                                            target_duty, time_ms);
    if (ret != ESP_OK) {
        return ret;
    }
    return ledc_fade_start(LEDC_LOW_SPEED_MODE, BSP_PWM_LEDC_CHANNEL, LEDC_FADE_WAIT_DONE);
}
