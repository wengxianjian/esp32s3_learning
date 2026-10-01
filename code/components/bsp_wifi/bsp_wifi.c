#include "bsp_wifi.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static const char *TAG = "bsp_wifi";

static EventGroupHandle_t s_sta_eg = NULL;
#define WIFI_CONNECTED_BIT BIT0

static void sta_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *evt = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "got IP: " IPSTR, IP2STR(&evt->ip_info.ip));
        xEventGroupSetBits(s_sta_eg, WIFI_CONNECTED_BIT);
    }
}

esp_err_t bsp_wifi_sta_connect(void)
{
    esp_err_t ret = bsp_wifi_init();
    if (ret != ESP_OK) return ret;

    s_sta_eg = xEventGroupCreate();
    esp_netif_create_default_wifi_sta();

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &sta_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &sta_event_handler, NULL);

    wifi_config_t cfg = { .sta = {
        .ssid = CONFIG_WIFI_STA_SSID,
        .password = CONFIG_WIFI_STA_PASS,
    }};
    ESP_LOGI(TAG, "connecting to SSID=%s", CONFIG_WIFI_STA_SSID);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &cfg);
    esp_wifi_start();

    xEventGroupWaitBits(s_sta_eg, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi STA connected");
    return ESP_OK;
}

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
