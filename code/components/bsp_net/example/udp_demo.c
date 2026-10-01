#include "bsp_net.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "udp_demo";

void bsp_udp_demo(void)
{
    ESP_LOGI(TAG, "UDP demo start");
    ESP_ERROR_CHECK(bsp_net_wifi_connect());

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
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
    ESP_LOGI(TAG, "UDP listening on port %d", CONFIG_SOCKET_PORT);

    uint8_t rxbuf[128];
    struct sockaddr_in remote;
    socklen_t remote_len = sizeof(remote);
    while (1) {
        int len = recvfrom(sock, rxbuf, sizeof(rxbuf) - 1, 0,
                           (struct sockaddr *)&remote, &remote_len);
        if (len > 0) {
            rxbuf[len] = 0;
            ESP_LOGI(TAG, "recv %d bytes: %s", len, rxbuf);
            /* 回显 */
            sendto(sock, rxbuf, len, 0, (struct sockaddr *)&remote, remote_len);
        }
    }
}
