#include "bsp_net.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include <string.h>

static const char *TAG = "tcp_server_demo";

void bsp_tcp_server_demo(void)
{
    ESP_LOGI(TAG, "TCP server demo start");
    ESP_ERROR_CHECK(bsp_net_wifi_connect());

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket failed");
        return;
    }

    struct sockaddr_in local = {
        .sin_family = AF_INET,
        .sin_port = htons(CONFIG_SOCKET_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    if (bind(sock, (struct sockaddr *)&local, sizeof(local)) < 0) {
        ESP_LOGE(TAG, "bind failed");
        return;
    }
    listen(sock, 5);
    ESP_LOGI(TAG, "TCP server listening on port %d", CONFIG_SOCKET_PORT);

    uint8_t rxbuf[128];
    while (1) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);
        int csock = accept(sock, (struct sockaddr *)&client, &client_len);
        if (csock < 0) continue;
        ESP_LOGI(TAG, "client connected");
        while (1) {
            int len = recv(csock, rxbuf, sizeof(rxbuf) - 1, 0);
            if (len <= 0) {
                ESP_LOGI(TAG, "client disconnected");
                break;
            }
            rxbuf[len] = 0;
            ESP_LOGI(TAG, "recv: %s", rxbuf);
            send(csock, rxbuf, len, 0);   /* 回显 */
        }
        close(csock);
    }
}
