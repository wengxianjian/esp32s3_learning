#include "bsp_uart.h"
#include "board.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "bsp_uart";

esp_err_t bsp_uart_init(void)
{
    uart_config_t cfg = {
        .baud_rate = BSP_UART0_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t ret = uart_param_config(BSP_UART0_NUM, &cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_set_pin(BSP_UART0_NUM, BSP_UART0_TX_GPIO, BSP_UART0_RX_GPIO,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_driver_install(BSP_UART0_NUM, 2048, 2048, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "UART0 init: %d baud, TX=GPIO%d RX=GPIO%d",
             BSP_UART0_BAUD_RATE, BSP_UART0_TX_GPIO, BSP_UART0_RX_GPIO);
    return ESP_OK;
}

int bsp_uart_send(const uint8_t *data, size_t len)
{
    return uart_write_bytes(BSP_UART0_NUM, data, len);
}

int bsp_uart_recv(uint8_t *buf, size_t len, TickType_t timeout)
{
    return uart_read_bytes(BSP_UART0_NUM, buf, len, timeout);
}
