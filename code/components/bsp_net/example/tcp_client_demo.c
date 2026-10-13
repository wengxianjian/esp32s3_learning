/**
 * @file tcp_client_demo.c
 * @brief TCP 客户端实验示例（对应指南第五十三章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 先 bsp_net_wifi_connect() 连上 WiFi 拿到 IP；
 * 2) 创建 TCP 套接字（SOCK_STREAM），connect 到 PC 端服务器（三次握手）；
 * 3) 死循环：发送一行 "ALIENTEK DATA\r\n"，再 recv 读取服务器回包并打印；
 * 4) 每轮间隔 2 秒（vTaskDelay 2000ms）。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "TCP client"（对应宏 CONFIG_DEMO_TCP_CLIENT），
 * 目标地址走 "Socket Config" 的 "Remote server IP"（缺省 192.168.1.100）
 * 与 "Socket port"（缺省 8080）。
 * PC 端需先运行 TCP 服务器（网络调试助手选 TCP Server，监听 8080），并把
 * Remote server IP 改成 PC 的实际 IP；app_main() 调用 bsp_tcp_client_demo()。
 */

#include "bsp_net.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/* 日志标签：本示例日志显示为 "tcp_client_demo" */
static const char *TAG = "tcp_client_demo";

void bsp_tcp_client_demo(void)
{
    ESP_LOGI(TAG, "TCP client demo start");
    /* 先联网拿到 IP（内部阻塞直到连接成功） */
    ESP_ERROR_CHECK(bsp_net_wifi_connect());

    /* 创建 TCP 套接字：SOCK_STREAM 即 TCP */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket failed");
        return;
    }

    /* 服务器地址：IP 与端口都来自 menuconfig（CONFIG_SOCKET_*） */
    struct sockaddr_in server = {
        .sin_family = AF_INET,
        .sin_port = htons(CONFIG_SOCKET_PORT),                     /* 端口转网络字节序 */
        .sin_addr.s_addr = inet_addr(CONFIG_SOCKET_REMOTE_IP),     /* 点分十进制转 32 位 */
    };
    /* TCP 三次握手连接服务器，失败则打印目标地址并退出 */
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        ESP_LOGE(TAG, "connect to %s:%d failed", CONFIG_SOCKET_REMOTE_IP, CONFIG_SOCKET_PORT);
        return;
    }
    ESP_LOGI(TAG, "connected to %s:%d", CONFIG_SOCKET_REMOTE_IP, CONFIG_SOCKET_PORT);

    const char *msg = "ALIENTEK DATA\r\n";   /* 周期性发送的一行数据（含回车换行） */
    uint8_t rxbuf[128];
    while (1) {
        /* 发送固定内容，长度用 strlen 计算 */
        send(sock, msg, strlen(msg), 0);
        /* 读取服务器回包，sizeof-1 预留结尾 '\0' */
        int len = recv(sock, rxbuf, sizeof(rxbuf) - 1, 0);
        if (len > 0) {
            rxbuf[len] = 0;
            ESP_LOGI(TAG, "recv: %s", rxbuf);
        }
        /* 每 2 秒发送一次，避免刷屏（1000 tick/秒 × 2） */
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
