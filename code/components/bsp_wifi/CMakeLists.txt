idf_component_register(
    SRCS "bsp_wifi.c"
         "example/wifi_scan_demo.c" "example/wifi_sta_demo.c"
         "example/wifi_ap_demo.c" "example/wifi_smartconfig_demo.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_common esp_wifi esp_netif esp_event nvs_flash
)
