#include "bsp_spiffs.h"
#include "board.h"
#include "esp_spiffs.h"
#include "esp_log.h"

static const char *TAG = "bsp_spiffs";

esp_err_t bsp_spiffs_init(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = BSP_SPIFFS_MOUNT_POINT,
        .partition_label = BSP_SPIFFS_PARTITION,
        .max_files = 5,
        .format_if_mount_failed = true,
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(BSP_SPIFFS_PARTITION, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS mount OK: total=%d, used=%d", total, used);
    }
    return ret;
}
