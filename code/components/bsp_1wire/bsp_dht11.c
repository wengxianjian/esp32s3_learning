#include "bsp_dht11.h"
#include "board.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

static const char *TAG = "bsp_dht11";

#define DQ gpio_get_level(BSP_1WIRE_GPIO)

static void dq_out(int level)
{
    gpio_set_level(BSP_1WIRE_GPIO, level);
}

static void dht11_reset(void)
{
    dq_out(0);
    vTaskDelay(pdMS_TO_TICKS(25));   /* 拉低至少 18ms */
    dq_out(1);
    esp_rom_delay_us(30);            /* 主机拉高 10~35us */
}

static int dht11_check(void)
{
    uint8_t retry = 0;
    while (DQ && retry < 100) {      /* 等 DHT11 拉低 ~83us */
        retry++;
        esp_rom_delay_us(1);
    }
    if (retry >= 100) return 1;
    retry = 0;
    while (!DQ && retry < 100) {     /* 等 DHT11 拉高 ~87us */
        retry++;
        esp_rom_delay_us(1);
    }
    return (retry >= 100) ? 1 : 0;
}

static uint8_t dht11_read_bit(void)
{
    uint8_t retry = 0;
    while (DQ && retry < 100) {      /* 等总线拉低 */
        retry++;
        esp_rom_delay_us(1);
    }
    retry = 0;
    while (!DQ && retry < 100) {     /* 等高电平开始 */
        retry++;
        esp_rom_delay_us(1);
    }
    esp_rom_delay_us(40);            /* 40us 后采样 */
    return DQ ? 1 : 0;               /* 高=1，低=0 */
}

static uint8_t dht11_read_byte(void)
{
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        data <<= 1;
        data |= dht11_read_bit();
    }
    return data;
}

esp_err_t bsp_dht11_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << BSP_1WIRE_GPIO,
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&cfg);
    if (ret != ESP_OK) return ret;

    dht11_reset();
    if (dht11_check() != 0) {
        ESP_LOGW(TAG, "DHT11 not present");
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "DHT11 init OK (DQ=GPIO%d)", BSP_1WIRE_GPIO);
    return ESP_OK;
}

esp_err_t bsp_dht11_read(uint8_t *temp, uint8_t *humi)
{
    uint8_t buf[5] = {0};

    dht11_reset();
    if (dht11_check() != 0) {
        ESP_LOGW(TAG, "DHT11 no response");
        return ESP_ERR_TIMEOUT;
    }
    for (int i = 0; i < 5; i++) {
        buf[i] = dht11_read_byte();
    }
    if ((uint8_t)(buf[0] + buf[1] + buf[2] + buf[3]) != buf[4]) {
        ESP_LOGW(TAG, "DHT11 checksum error");
        return ESP_ERR_INVALID_CRC;
    }
    *humi = buf[0];
    *temp = buf[2];
    return ESP_OK;
}
