/**
 * @file udp_demo.c
 * @brief UDP 回显服务器实验示例（对应指南第五十二章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 先 bsp_net_wifi_connect() 连上 WiFi 拿到 IP；
 * 2) 创建 UDP 套接字（SOCK_DGRAM），bind 到本地 CONFIG_SOCKET_PORT 端口；
 * 3) 死循环 recvfrom() 收数据，把收到的内容以日志打印；
 * 4) 再 sendto() 把原样数据回发（回显）给发送方。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "UDP echo server"（对应宏 CONFIG_DEMO_UDP），
 * 端口走 "Socket Config" 的 "Socket port"（缺省 8080），app_main() 调用 bsp_udp_demo()。
 * PC 端用「网络调试助手」选 UDP，目标填开发板串口打印的 IP、端口 8080，发送即可看到回显。
 */

#include "bsp_net.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/* 日志标签：本示例日志显示为 "udp_demo" */
static const char *TAG = "udp_demo";

void bsp_udp_demo(void)
{
    ESP_LOGI(TAG, "UDP demo start");
    /* 先联网拿到 IP（内部阻塞直到连接成功） */
    ESP_ERROR_CHECK(bsp_net_wifi_connect());

    /* 创建 UDP 套接字：AF_INET=IPv4，SOCK_DGRAM=UDP，0=默认协议 */
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket failed");
        return;
    }

    /* 本地地址：绑定本机任意网卡的 CONFIG_SOCKET_PORT 端口 */
    struct sockaddr_in local = {
        .sin_family = AF_INET,
        .sin_port = htons(CONFIG_SOCKET_PORT),   /* 端口需转成网络字节序（大端） */
        .sin_addr.s_addr = htonl(INADDR_ANY),    /* INADDR_ANY=监听本机所有 IP */
    };
    if (bind(sock, (struct sockaddr *)&local, sizeof(local)) < 0) {
        ESP_LOGE(TAG, "bind failed");
        return;
    }
    ESP_LOGI(TAG, "UDP listening on port %d", CONFIG_SOCKET_PORT);

    uint8_t rxbuf[128];                 /* 接收缓冲，一次最多收 127 字节+结尾 '\0' */
    struct sockaddr_in remote;          /* 出参：发送方地址，回显时要用 */
    socklen_t remote_len = sizeof(remote);
    while (1) {
        /* 阻塞收包：sizeof-1 预留 1 字节放字符串结尾 '\0' */
        int len = recvfrom(sock, rxbuf, sizeof(rxbuf) - 1, 0,
                           (struct sockaddr *)&remote, &remote_len);
        if (len > 0) {
            rxbuf[len] = 0;             /* 手动补字符串结束符，便于 %s 打印 */
            ESP_LOGI(TAG, "recv %d bytes: %s", len, rxbuf);
            /* 回显：把收到的 len 字节原样发回给发送方地址 remote */
            sendto(sock, rxbuf, len, 0, (struct sockaddr *)&remote, remote_len);
        }
    }
}
