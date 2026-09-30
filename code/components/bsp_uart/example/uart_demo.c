#include "bsp_uart.h"
#include "bsp_led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

static const char *TAG = "uart_demo";

void bsp_uart_demo(void)
{
    ESP_LOGI(TAG, "UART demo start: periodic msg + echo (monitor CH340 port)");
    ESP_ERROR_CHECK(bsp_uart_init());
    ESP_ERROR_CHECK(bsp_led_init());

    uint8_t rx_buf[128];
    uint32_t tick = 0;

    while (1) {
        /* 定时发送提示信息 */
        char msg[64];
        int n = snprintf(msg, sizeof(msg), "Hello UART, tick = %lu\r\n",
                         (unsigned long)tick++);
        bsp_uart_send((const uint8_t *)msg, n);

        /* 回显收到的数据 */
        int len = bsp_uart_recv(rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(50));
        if (len > 0) {
            bsp_uart_send(rx_buf, len);
        }

        bsp_led_toggle();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
