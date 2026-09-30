#include "bsp_pwm.h"
#include "board.h"
#include "esp_log.h"

static const char *TAG = "hwpwm_demo";

void bsp_hwpwm_demo(void)
{
    ESP_LOGI(TAG, "HW_PWM demo start: LED breathing (hardware fade)");
    ESP_ERROR_CHECK(bsp_pwm_init());

    while (1) {
        /* 渐亮 */
        ESP_ERROR_CHECK(bsp_pwm_fade_to(BSP_PWM_DUTY_MAX, 1000));
        /* 渐暗 */
        ESP_ERROR_CHECK(bsp_pwm_fade_to(0, 1000));
    }
}
