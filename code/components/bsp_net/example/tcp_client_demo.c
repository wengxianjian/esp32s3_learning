#include "bsp_net.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "tcp_client_demo";

void bsp_tcp_client_demo(void)
{
    ESP_LOGI(TAG, "TCP client demo start");
    ESP_ERROR_CHECK(bsp_net_wifi_connect());

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket failed");
        return;
    }

    struct sockaddr_in server = {
        .sin_family = AF_INET,
        .sin_port = htons(CONFIG_SOCKET_PORT),
        .sin_addr.s_addr = inet_addr(CONFIG_SOCKET_REMOTE_IP),
    };
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        ESP_LOGE(TAG, "connect to %s:%d failed", CONFIG_SOCKET_REMOTE_IP, CONFIG_SOCKET_PORT);
        return;
    }
    ESP_LOGI(TAG, "connected to %s:%d", CONFIG_SOCKET_REMOTE_IP, CONFIG_SOCKET_PORT);

    const char *msg = "ALIENTEK DATA\r\n";
    uint8_t rxbuf[128];
    while (1) {
        send(sock, msg, strlen(msg), 0);
        int len = recv(sock, rxbuf, sizeof(rxbuf) - 1, 0);
        if (len > 0) {
            rxbuf[len] = 0;
            ESP_LOGI(TAG, "recv: %s", rxbuf);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
