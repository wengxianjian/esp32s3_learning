#include "bsp_wifi.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"

static const char *TAG = "bsp_wifi";

esp_err_t bsp_wifi_init(void)
{
    /* NVS（WiFi 校准数据） */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) return ret;

    /* 网络接口 + 事件循环 */
    esp_netif_init();
    esp_event_loop_create_default();

    /* WiFi 驱动 */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) return ret;

    ESP_LOGI(TAG, "WiFi init OK");
    return ESP_OK;
}
