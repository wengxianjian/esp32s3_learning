#include "bsp_ir_rx.h"
#include "board.h"
#include "driver/rmt_rx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "bsp_ir_rx";

static rmt_channel_handle_t s_rx_channel = NULL;
static QueueHandle_t s_rx_queue = NULL;

#define NEC_LEADING_0   9000
#define NEC_LEADING_1   4500
#define NEC_ZERO_0      560
#define NEC_ZERO_1      560
#define NEC_ONE_0       560
#define NEC_ONE_1       1690

static inline bool in_range(uint32_t d, uint32_t spec)
{
    return (d < spec + BSP_IR_NEC_DECODE_MARGIN) && (d > spec - BSP_IR_NEC_DECODE_MARGIN);
}

static bool parse_logic0(rmt_symbol_word_t *s)
{
    return in_range(s->duration0, NEC_ZERO_0) && in_range(s->duration1, NEC_ZERO_1);
}

static bool parse_logic1(rmt_symbol_word_t *s)
{
    return in_range(s->duration0, NEC_ONE_0) && in_range(s->duration1, NEC_ONE_1);
}

static bool nec_parse_frame(rmt_symbol_word_t *symbols, uint16_t *address, uint16_t *command)
{
    rmt_symbol_word_t *cur = symbols;
    if (!in_range(cur->duration0, NEC_LEADING_0) || !in_range(cur->duration1, NEC_LEADING_1)) {
        return false;
    }
    cur++;
    uint16_t addr = 0, cmd = 0;
    for (int i = 0; i < 16; i++) {
        if (parse_logic1(cur)) addr |= 1 << i;
        else if (parse_logic0(cur)) addr &= ~(1 << i);
        else return false;
        cur++;
    }
    for (int i = 0; i < 16; i++) {
        if (parse_logic1(cur)) cmd |= 1 << i;
        else if (parse_logic0(cur)) cmd &= ~(1 << i);
        else return false;
        cur++;
    }
    *address = addr;
    *command = cmd;
    return true;
}

static bool rx_done_cb(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data)
{
    BaseType_t w = pdFALSE;
    xQueueSendFromISR((QueueHandle_t)user_data, edata, &w);
    return w == pdTRUE;
}

static void start_receive(void)
{
    rmt_receive_config_t cfg = {
        .signal_range_min_ns = 1250,
        .signal_range_max_ns = 12000000,
    };
    rmt_symbol_word_t symbols[64];
    rmt_receive(s_rx_channel, symbols, sizeof(symbols), &cfg);
}

esp_err_t bsp_ir_rx_init(void)
{
    rmt_rx_channel_config_t cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = BSP_IR_RESOLUTION_HZ,
        .mem_block_symbols = 64,
        .gpio_num = BSP_IR_RX_GPIO,
    };
    esp_err_t ret = rmt_new_rx_channel(&cfg, &s_rx_channel);
    if (ret != ESP_OK) return ret;

    s_rx_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    rmt_rx_event_callbacks_t cbs = { .on_recv_done = rx_done_cb };
    rmt_rx_register_event_callbacks(s_rx_channel, &cbs, s_rx_queue);
    rmt_enable(s_rx_channel);
    start_receive();

    ESP_LOGI(TAG, "IR RX init OK (GPIO%d)", BSP_IR_RX_GPIO);
    return ESP_OK;
}

esp_err_t bsp_ir_rx_receive(uint16_t *address, uint16_t *command, uint32_t timeout_ms)
{
    rmt_rx_done_event_data_t rx_data;
    if (xQueueReceive(s_rx_queue, &rx_data, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t ret = ESP_ERR_INVALID_RESPONSE;
    if (nec_parse_frame(rx_data.received_symbols, address, command)) {
        ret = ESP_OK;
    }
    start_receive();   /* 继续接收下一帧 */
    return ret;
}
