idf_component_register(
    SRCS "bsp_net.c" "example/udp_demo.c" "example/tcp_client_demo.c" "example/tcp_server_demo.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_common bsp_wifi esp_netif lwip
)
