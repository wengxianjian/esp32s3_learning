/**
 * @file tcp_server_demo.c
 * @brief TCP 服务器（回显）实验示例（对应指南第五十四章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 先 bsp_net_wifi_connect() 连上 WiFi 拿到 IP；
 * 2) 创建 TCP 套接字，bind 到本地 CONFIG_SOCKET_PORT，listen 监听（backlog=5）；
 * 3) accept() 阻塞等待客户端接入；
 * 4) 对每个已连接客户端循环 recv，收到就 send 原样回发（回显），断开则关连接；
 * 5) 处理完后回到 accept，等待下一个客户端。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "TCP server (echo)"（对应宏 CONFIG_DEMO_TCP_SERVER），
 * 端口走 "Socket Config" 的 "Socket port"（缺省 8080），app_main() 调用 bsp_tcp_server_demo()。
 * PC 端用「网络调试助手」选 TCP Client，目标填开发板 IP、端口 8080，连接后发送即可看到回显。
 */

#include "bsp_net.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include <string.h>

/* 日志标签：本示例日志显示为 "tcp_server_demo" */
static const char *TAG = "tcp_server_demo";

void bsp_tcp_server_demo(void)
{
    ESP_LOGI(TAG, "TCP server demo start");
    /* 先联网拿到 IP（内部阻塞直到连接成功） */
    ESP_ERROR_CHECK(bsp_net_wifi_connect());

    /* 创建 TCP 监听套接字 */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket failed");
        return;
    }

    /* 绑定本机任意 IP 的 CONFIG_SOCKET_PORT 端口 */
    struct sockaddr_in local = {
        .sin_family = AF_INET,
        .sin_port = htons(CONFIG_SOCKET_PORT),   /* 端口转网络字节序 */
        .sin_addr.s_addr = htonl(INADDR_ANY),    /* 监听本机所有 IP */
    };
    if (bind(sock, (struct sockaddr *)&local, sizeof(local)) < 0) {
        ESP_LOGE(TAG, "bind failed");
        return;
    }
    /* 开始监听：第二个参数 5 是等待接受的最大连接数（backlog） */
    listen(sock, 5);
    ESP_LOGI(TAG, "TCP server listening on port %d", CONFIG_SOCKET_PORT);

    uint8_t rxbuf[128];
    while (1) {
        struct sockaddr_in client;              /* 出参：接入的客户端地址 */
        socklen_t client_len = sizeof(client);
        /* 阻塞等待客户端接入，返回一个专用于该连接的新套接字 csock */
        int csock = accept(sock, (struct sockaddr *)&client, &client_len);
        if (csock < 0) continue;                /* 接入失败则继续等下一个 */
        ESP_LOGI(TAG, "client connected");
        while (1) {
            int len = recv(csock, rxbuf, sizeof(rxbuf) - 1, 0);
            /* 返回 <=0 说明对端关闭或出错，退出内层循环处理下一个客户端 */
            if (len <= 0) {
                ESP_LOGI(TAG, "client disconnected");
                break;
            }
            rxbuf[len] = 0;                     /* 补字符串结尾，便于 %s 打印 */
            ESP_LOGI(TAG, "recv: %s", rxbuf);
            send(csock, rxbuf, len, 0);   /* 回显 */
        }
        close(csock);                           /* 关闭该客户端连接，释放资源 */
    }
}
