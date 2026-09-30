#include "bsp_i2c.h"
#include "board.h"
#include "esp_log.h"

static const char *TAG = "bsp_xl9555";

/* XL9555 寄存器 */
#define XL9555_INPUT_PORT0_REG    0
#define XL9555_INPUT_PORT1_REG    1
#define XL9555_OUTPUT_PORT0_REG   2
#define XL9555_OUTPUT_PORT1_REG   3
#define XL9555_CONFIG_PORT0_REG   6
#define XL9555_CONFIG_PORT1_REG   7

static uint16_t s_output = 0;
static uint16_t s_config = 0;

esp_err_t bsp_xl9555_init(void)
{
    esp_err_t ret = bsp_i2c_init();
    if (ret != ESP_OK) return ret;

    /* 配置：P00/P01(AP_INT/QMA_INT)、P14~P17(KEY0~KEY3) 为输入，其余为输出 */
    s_config = 0xF003;
    ret = bsp_i2c_write_reg(BSP_XL9555_ADDR, XL9555_CONFIG_PORT0_REG,
                            (uint8_t[]){s_config & 0xFF}, 1);
    if (ret != ESP_OK) return ret;
    ret = bsp_i2c_write_reg(BSP_XL9555_ADDR, XL9555_CONFIG_PORT1_REG,
                            (uint8_t[]){(s_config >> 8) & 0xFF}, 1);
    if (ret != ESP_OK) return ret;

    /* 初始关闭蜂鸣器 */
    bsp_xl9555_pin_write(BSP_XL9555_BEEP_IO, 0);

    ESP_LOGI(TAG, "XL9555 init OK (addr 0x%02x)", BSP_XL9555_ADDR);
    return ESP_OK;
}

void bsp_xl9555_pin_write(uint16_t pin, int val)
{
    if (val) {
        s_output |= pin;
    } else {
        s_output &= ~pin;
    }
    bsp_i2c_write_reg(BSP_XL9555_ADDR, XL9555_OUTPUT_PORT0_REG,
                      (uint8_t[]){s_output & 0xFF}, 1);
    bsp_i2c_write_reg(BSP_XL9555_ADDR, XL9555_OUTPUT_PORT1_REG,
                      (uint8_t[]){(s_output >> 8) & 0xFF}, 1);
}

int bsp_xl9555_pin_read(uint16_t pin)
{
    uint8_t data[2] = {0};
    bsp_i2c_read_reg(BSP_XL9555_ADDR, XL9555_INPUT_PORT0_REG, data, 2);
    uint16_t input = (data[1] << 8) | data[0];
    return (input & pin) ? 1 : 0;
}
