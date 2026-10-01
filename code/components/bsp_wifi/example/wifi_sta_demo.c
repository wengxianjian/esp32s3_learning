#include "bsp_wifi.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "wifi_sta_demo";
#define WIFI_CONNECTED_BIT BIT0

static EventGroupHandle_t s_wifi_eg = NULL;

static void sta_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "disconnected, retrying...");
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *evt = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "got IP: " IPSTR, IP2STR(&evt->ip_info.ip));
        xEventGroupSetBits(s_wifi_eg, WIFI_CONNECTED_BIT);
    }
}

void bsp_wifi_sta_demo(void)
{
    ESP_LOGI(TAG, "WiFi STA demo start");
    ESP_ERROR_CHECK(bsp_wifi_init());

    s_wifi_eg = xEventGroupCreate();
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

    xEventGroupWaitBits(s_wifi_eg, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi STA connected");
}
