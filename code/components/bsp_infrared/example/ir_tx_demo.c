#include "bsp_ir_tx.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ir_tx_demo";

void bsp_ir_tx_demo(void)
{
    ESP_LOGI(TAG, "IR TX demo start");
    ESP_ERROR_CHECK(bsp_ir_tx_init());

    uint16_t address = 0x0440;
    uint16_t command = 0x3003;
    while (1) {
        ESP_ERROR_CHECK(bsp_ir_tx_send(address, command));
        ESP_LOGI(TAG, "sent Address=0x%04X Command=0x%04X", address, command);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
