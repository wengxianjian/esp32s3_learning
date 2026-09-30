#include "bsp_ir_tx.h"
#include "ir_nec_encoder.h"
#include "board.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"

static const char *TAG = "bsp_ir_tx";

static rmt_channel_handle_t s_tx_channel = NULL;
static rmt_encoder_handle_t s_nec_encoder = NULL;

esp_err_t bsp_ir_tx_init(void)
{
    rmt_tx_channel_config_t cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = BSP_IR_RESOLUTION_HZ,
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
        .gpio_num = BSP_IR_TX_GPIO,
    };
    esp_err_t ret = rmt_new_tx_channel(&cfg, &s_tx_channel);
    if (ret != ESP_OK) return ret;

    rmt_carrier_config_t carrier = {
        .duty_cycle = 0.33,
        .frequency_hz = 38000,
    };
    rmt_apply_carrier(s_tx_channel, &carrier);

    ir_nec_encoder_config_t enc_cfg = { .resolution = BSP_IR_RESOLUTION_HZ };
    ret = rmt_new_ir_nec_encoder(&enc_cfg, &s_nec_encoder);
    if (ret != ESP_OK) return ret;

    rmt_enable(s_tx_channel);
    ESP_LOGI(TAG, "IR TX init OK (GPIO%d, 38kHz)", BSP_IR_TX_GPIO);
    return ESP_OK;
}

esp_err_t bsp_ir_tx_send(uint16_t address, uint16_t command)
{
    ir_nec_scan_code_t code = { .address = address, .command = command };
    rmt_transmit_config_t cfg = { .loop_count = 0 };
    return rmt_transmit(s_tx_channel, s_nec_encoder, &code, sizeof(code), &cfg);
}
