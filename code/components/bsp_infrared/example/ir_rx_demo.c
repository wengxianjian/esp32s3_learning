#include "bsp_ir_rx.h"
#include "esp_log.h"

static const char *TAG = "ir_rx_demo";

void bsp_ir_rx_demo(void)
{
    ESP_LOGI(TAG, "IR RX demo start");
    ESP_ERROR_CHECK(bsp_ir_rx_init());

    uint16_t address, command;
    while (1) {
        esp_err_t ret = bsp_ir_rx_receive(&address, &command, 60000);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Address=0x%04X, Command=0x%04X", address, command);
        }
    }
}
