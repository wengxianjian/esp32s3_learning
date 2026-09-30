#include "bsp_ap3216c.h"
#include "bsp_i2c.h"
#include "board.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "bsp_ap3216c";

#define AP3216C_REG_SYSCONF   0x00   /* 系统配置寄存器 */
#define AP3216C_REG_DATA      0x0A   /* 数据寄存器基址（连续 6 字节） */

esp_err_t bsp_ap3216c_init(void)
{
    esp_err_t ret = bsp_i2c_init();
    if (ret != ESP_OK) return ret;

    uint8_t v = 0x04;   /* 软件复位 */
    bsp_i2c_write_reg(BSP_AP3216C_ADDR, AP3216C_REG_SYSCONF, &v, 1);
    vTaskDelay(pdMS_TO_TICKS(50));   /* 复位至少 10ms */

    v = 0x03;           /* 使能 ALS + PS + IR */
    bsp_i2c_write_reg(BSP_AP3216C_ADDR, AP3216C_REG_SYSCONF, &v, 1);

    uint8_t tmp = 0;
    bsp_i2c_read_reg(BSP_AP3216C_ADDR, AP3216C_REG_SYSCONF, &tmp, 1);
    if (tmp != 0x03) {
        ESP_LOGE(TAG, "check fail (read 0x%02x)", tmp);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "AP3216C init OK");
    return ESP_OK;
}

esp_err_t bsp_ap3216c_read(uint16_t *ir, uint16_t *als, uint16_t *ps)
{
    uint8_t buf[6] = {0};
    esp_err_t ret = bsp_i2c_read_reg(BSP_AP3216C_ADDR, AP3216C_REG_DATA, buf, 6);
    if (ret != ESP_OK) return ret;

    /* IR：IR_OF 置 1 表示溢出，数据无效 */
    if (buf[0] & 0x80) {
        *ir = 0;
    } else {
        *ir = ((uint16_t)buf[1] << 2) | (buf[0] & 0x03);
    }

    /* ALS：高字节 buf[3]，低字节 buf[2] */
    *als = ((uint16_t)buf[3] << 8) | buf[2];

    /* PS：PS_OF 置 1 表示溢出，数据无效 */
    if (buf[4] & 0x40) {
        *ps = 0;
    } else {
        *ps = ((uint16_t)(buf[5] & 0x3F) << 4) | (buf[4] & 0x0F);
    }
    return ESP_OK;
}
