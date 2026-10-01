#include "bsp_wifi.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "wifi_ap_demo";

void bsp_wifi_ap_demo(void)
{
    ESP_LOGI(TAG, "WiFi AP demo start");
    ESP_ERROR_CHECK(bsp_wifi_init());

    esp_netif_create_default_wifi_ap();

    wifi_config_t cfg = { .ap = {
        .ssid = CONFIG_WIFI_AP_SSID,
        .ssid_len = strlen(CONFIG_WIFI_AP_SSID),
        .password = CONFIG_WIFI_AP_PASS,
        .max_connection = 4,
        .authmode = WIFI_AUTH_WPA_WPA2_PSK,
    }};

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &cfg);
    esp_wifi_start();

    ESP_LOGI(TAG, "AP started: SSID=%s", CONFIG_WIFI_AP_SSID);
}
