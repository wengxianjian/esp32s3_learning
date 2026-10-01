idf_component_register(
    SRCS "bsp_mqtt.c" "example/mqtt_demo.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_common bsp_wifi mqtt esp_netif
)
