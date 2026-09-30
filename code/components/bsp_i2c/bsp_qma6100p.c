#include "bsp_qma6100p.h"
#include "bsp_i2c.h"
#include "board.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "bsp_qma6100p";

/* 寄存器地址 */
#define QMA_REG_CHIP_ID      0x00
#define QMA_REG_XOUTL        0x01
#define QMA_REG_RANGE        0x0F
#define QMA_REG_BW_ODR       0x10
#define QMA_REG_POWER        0x11
#define QMA_REG_RESET        0x36

static esp_err_t qma_write(uint8_t reg, uint8_t val)
{
    return bsp_i2c_write_reg(BSP_QMA6100P_ADDR, reg, &val, 1);
}

esp_err_t bsp_qma6100p_init(void)
{
    esp_err_t ret = bsp_i2c_init();
    if (ret != ESP_OK) return ret;

    /* 复位：0xB6 → 0x00 */
    qma_write(QMA_REG_RESET, 0xB6);
    vTaskDelay(pdMS_TO_TICKS(5));
    qma_write(QMA_REG_RESET, 0x00);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* 初始化序列（参考规格书 1a 节） */
    qma_write(0x11, 0x80);
    qma_write(0x11, 0x84);
    qma_write(0x4A, 0x20);
    qma_write(0x56, 0x01);
    qma_write(0x5F, 0x80);
    vTaskDelay(pdMS_TO_TICKS(1));
    qma_write(0x5F, 0x00);
    vTaskDelay(pdMS_TO_TICKS(10));

    qma_write(QMA_REG_RANGE, 0x04);     /* 量程 8G */
    qma_write(QMA_REG_BW_ODR, 0x00);    /* 带宽 100Hz */
    qma_write(QMA_REG_POWER, 0x84);     /* MCLK 51.2K | 激活 */
    qma_write(0x21, 0x03);

    uint8_t id = 0;
    bsp_i2c_read_reg(BSP_QMA6100P_ADDR, QMA_REG_CHIP_ID, &id, 1);
    if (id != 0x90) {
        ESP_LOGE(TAG, "CHIP_ID check fail (read 0x%02x)", id);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "QMA6100P init OK");
    return ESP_OK;
}

esp_err_t bsp_qma6100p_read_raw(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t buf[6] = {0};
    esp_err_t ret = bsp_i2c_read_reg(BSP_QMA6100P_ADDR, QMA_REG_XOUTL, buf, 6);
    if (ret != ESP_OK) return ret;

    *x = (int16_t)(((uint16_t)buf[1] << 8) | buf[0]) >> 2;
    *y = (int16_t)(((uint16_t)buf[3] << 8) | buf[2]) >> 2;
    *z = (int16_t)(((uint16_t)buf[5] << 8) | buf[4]) >> 2;
    return ESP_OK;
}
