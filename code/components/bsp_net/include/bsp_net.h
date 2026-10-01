/**
 * @file bsp_net.h
 * @brief lwIP Socket（UDP/TCP）板级接口
 */
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 连接 WiFi 并返回（供 socket demo 使用） */
esp_err_t bsp_net_wifi_connect(void);

/** UDP 示例（第五十二章） */
void bsp_udp_demo(void);

/** TCP Client 示例（第五十三章） */
void bsp_tcp_client_demo(void);

/** TCP Server 示例（第五十四章） */
void bsp_tcp_server_demo(void);

#ifdef __cplusplus
}
#endif
