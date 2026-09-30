#include "bsp_sdcard.h"
#include "board.h"
#include "driver/sdspi_host.h"
#include "driver/sdmmc_host.h"
#include "driver/spi_common.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"

static const char *TAG = "bsp_sdcard";

static sdmmc_card_t *s_card = NULL;

esp_err_t bsp_sdcard_init(void)
{
    /* 1. SPI2 总线 */
    spi_bus_config_t buscfg = {
        .mosi_io_num = BSP_SPI2_MOSI_GPIO,
        .miso_io_num = BSP_SPI2_MISO_GPIO,
        .sclk_io_num = BSP_SPI2_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 2. SDSPI host + slot */
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;

    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.host_id = SPI2_HOST;
    slot.gpio_cs = BSP_SDCARD_CS_GPIO;

    esp_vfs_fat_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    ret = esp_vfs_fat_sdspi_mount(BSP_SDCARD_MOUNT_POINT, &host, &slot, &mount_cfg, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "SD card mounted at %s", BSP_SDCARD_MOUNT_POINT);
    return ESP_OK;
}
