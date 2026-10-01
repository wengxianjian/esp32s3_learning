/**
 * @file bsp_mqtt.h
 * @brief MQTT 客户端板级接口
 */
#pragma once

#include "esp_err.h"
#include "mqtt_client.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 连接 WiFi 并启动 MQTT 客户端 */
esp_err_t bsp_mqtt_start(void);

/** 获取 MQTT 客户端句柄（供发布） */
esp_mqtt_client_handle_t bsp_mqtt_get_client(void);

/** MQTT 示例（第五十五章） */
void bsp_mqtt_demo(void);

#ifdef __cplusplus
}
#endif
