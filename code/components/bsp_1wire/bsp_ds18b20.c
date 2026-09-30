#include "bsp_ds18b20.h"
#include "board.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include <math.h>

static const char *TAG = "bsp_ds18b20";

#define DS18B20_CMD_SKIP_ROM     0xCC
#define DS18B20_CMD_CONVERT      0x44
#define DS18B20_CMD_READ_SCRATCH 0xBE

static void dq_out(int level)
{
    gpio_set_level(BSP_1WIRE_GPIO, level);
}

static int dq_in(void)
{
    return gpio_get_level(BSP_1WIRE_GPIO);
}

esp_err_t bsp_ds18b20_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << BSP_1WIRE_GPIO,
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&cfg);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "DS18B20 init OK (DQ=GPIO%d)", BSP_1WIRE_GPIO);
    }
    return ret;
}

/* 复位：拉低 480us 释放，检测存在脉冲 */
static int ds18b20_reset(void)
{
    dq_out(0);
    esp_rom_delay_us(480);
    dq_out(1);
    esp_rom_delay_us(60);
    int presence = dq_in();   /* 0 = 器件存在 */
    esp_rom_delay_us(240);
    return presence == 0;
}

static void ds18b20_write_bit(int bit)
{
    if (bit) {
        dq_out(0);
        esp_rom_delay_us(2);
        dq_out(1);
        esp_rom_delay_us(60);
    } else {
        dq_out(0);
        esp_rom_delay_us(60);
        dq_out(1);
        esp_rom_delay_us(2);
    }
}

static int ds18b20_read_bit(void)
{
    int bit = 0;
    dq_out(0);
    esp_rom_delay_us(2);
    dq_out(1);
    esp_rom_delay_us(15);
    bit = dq_in();
    esp_rom_delay_us(45);
    return bit;
}

static void ds18b20_write_byte(uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        ds18b20_write_bit(data & 0x01);
        data >>= 1;
    }
}

static uint8_t ds18b20_read_byte(void)
{
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        if (ds18b20_read_bit()) {
            data |= (1 << i);
        }
    }
    return data;
}

float bsp_ds18b20_read_temperature(void)
{
    if (!ds18b20_reset()) {
        ESP_LOGW(TAG, "DS18B20 not present");
        return NAN;
    }
    ds18b20_write_byte(DS18B20_CMD_SKIP_ROM);
    ds18b20_write_byte(DS18B20_CMD_CONVERT);
    vTaskDelay(pdMS_TO_TICKS(750));   /* 12 位转换约 750ms */

    if (!ds18b20_reset()) {
        ESP_LOGW(TAG, "DS18B20 not present (after convert)");
        return NAN;
    }
    ds18b20_write_byte(DS18B20_CMD_SKIP_ROM);
    ds18b20_write_byte(DS18B20_CMD_READ_SCRATCH);

    uint8_t lsb = ds18b20_read_byte();
    uint8_t msb = ds18b20_read_byte();
    int16_t raw = (int16_t)((msb << 8) | lsb);
    return raw / 16.0f;   /* 12 位，0.0625°C/LSB */
}
