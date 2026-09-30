#include "bsp_pwm.h"
#include "board.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "swpwm_demo";

void bsp_swpwm_demo(void)
{
    ESP_LOGI(TAG, "SW_PWM demo start: LED breathing (software duty change)");
    ESP_ERROR_CHECK(bsp_pwm_init());

    uint32_t duty = 0;
    int dir = 1;

    while (1) {
        duty += dir * 5;
        if (duty >= BSP_PWM_DUTY_MAX) {
            dir = -1;
        } else if (duty == 0) {
            dir = 1;
        }
        bsp_pwm_set_duty(duty);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
