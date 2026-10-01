#include "bsp_wifi.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_smartconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static const char *TAG = "wifi_smartconfig_demo";
#define SC_CONNECTED_BIT BIT0
#define SC_DONE_BIT      BIT1

static EventGroupHandle_t s_sc_eg = NULL;

static void sc_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_smartconfig_start(NULL);
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "disconnected, reconnect...");
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *evt = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "got IP: " IPSTR, IP2STR(&evt->ip_info.ip));
        xEventGroupSetBits(s_sc_eg, SC_CONNECTED_BIT);
    } else if (base == SC_EVENT && id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "got SSID/password via SmartConfig");
    } else if (base == SC_EVENT && id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(s_sc_eg, SC_DONE_BIT);
    }
}

void bsp_wifi_smartconfig_demo(void)
{
    ESP_LOGI(TAG, "WiFi SmartConfig demo start");
    ESP_ERROR_CHECK(bsp_wifi_init());

    s_sc_eg = xEventGroupCreate();
    esp_netif_create_default_wifi_sta();

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &sc_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &sc_event_handler, NULL);
    esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &sc_event_handler, NULL);

    esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    ESP_LOGI(TAG, "waiting for SmartConfig (use ESPTouch app)...");
    xEventGroupWaitBits(s_sc_eg, SC_CONNECTED_BIT | SC_DONE_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "SmartConfig done, WiFi connected");
}
