#include "bsp_wifi.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_log.h"
#include <stdlib.h>

static const char *TAG = "wifi_scan_demo";

void bsp_wifi_scan_demo(void)
{
    ESP_LOGI(TAG, "WiFi scan demo start");
    ESP_ERROR_CHECK(bsp_wifi_init());

    esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* 阻塞式扫描 */
    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));

    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);

    if (ap_count > 0) {
        wifi_ap_record_t *ap = calloc(ap_count, sizeof(wifi_ap_record_t));
        uint16_t number = ap_count;
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap));
        for (int i = 0; i < ap_count; i++) {
            ESP_LOGI(TAG, "[%d] SSID=%s RSSI=%d auth=%d",
                     i, ap[i].ssid, ap[i].rssi, ap[i].authmode);
        }
        free(ap);
    }
    ESP_LOGI(TAG, "WiFi scan done");
}
