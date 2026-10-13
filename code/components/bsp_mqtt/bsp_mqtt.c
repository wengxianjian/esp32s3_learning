/**
 * @file bsp_mqtt.c
 * @brief MQTT 客户端板级实现（基于 esp-mqtt，发布/订阅）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 本文件对外提供两个函数：
 *   bsp_mqtt_start()      —— 连 WiFi 并启动 MQTT 客户端
 *   bsp_mqtt_get_client() —— 取回客户端句柄（供外部发布消息）
 * 另含内部事件回调 mqtt_event_handler()，处理连接/订阅/发布/收数据等事件。
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * MQTT 是构架在 TCP 之上的「发布/订阅」协议，客户端连上 Broker 后用主题(topic)
 * 收发消息。这里用 ESP-IDF 的 esp_mqtt_client：
 *   ① 先 bsp_wifi_sta_connect() 联网（MQTT 走 TCP，必须已有 IP）；
 *   ② 组装配置：Broker 地址来自 menuconfig，client_id 固定 "esp32s3_learning"；
 *   ③ init → register_event（注册回调）→ start，客户端开始异步连接 Broker；
 *   ④ 连接成功（MQTT_EVENT_CONNECTED）时自动订阅并发布一条 "hello esp32-s3"；
 *   ⑤ 之后每收到消息触发 MQTT_EVENT_DATA，回调里打印主题与内容。
 * 收发全在回调里异步完成，故 start() 会立即返回、不阻塞。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * Broker URI 与主题来自 menuconfig：CONFIG_MQTT_BROKER_URI / CONFIG_MQTT_TOPIC
 * （对应 main/Kconfig 的 "MQTT Config"，缺省 mqtt://test.mosquitto.org、esp32s3/test）。
 * 指南章节：第五十五章 MQTT 实验。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * ① 必须先联网成功再 start，否则连不上 Broker。
 * ② 公共 broker 可能被占用或限流，连不上可换自建 broker。
 * ③ 回调运行在 MQTT 任务上下文，别在其中做长时间阻塞。
 * ④ 句柄 s_client 用 static 保存，供 bsp_mqtt_get_client() 复用。
 */

#include "bsp_mqtt.h"
#include "bsp_wifi.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/* 日志标签：MQTT 日志统一显示为 "bsp_mqtt" */
static const char *TAG = "bsp_mqtt";
/* MQTT 客户端句柄：由 bsp_mqtt_start() 创建，供发布时复用 */
static esp_mqtt_client_handle_t s_client = NULL;

/**
 * @brief MQTT 事件回调（注册到 MQTT 客户端的事件机制）
 *
 * 逻辑流程：
 *   ① CONNECTED    → 订阅主题并首发一条 hello
 *   ② DISCONNECTED → 告警（内部会自动重连）
 *   ③ SUBSCRIBED   → 打印订阅确认
 *   ④ PUBLISHED    → 打印发布确认
 *   ⑤ DATA         → 打印收到的主题与内容
 *   ⑥ ERROR        → 打印错误
 */
static void mqtt_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    /* 事件数据统一转成 MQTT 事件句柄，便于取 client / topic / data */
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    /* 按事件 ID 分派处理 */
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        /* 连上 Broker：订阅主题(QoS=0)，并首发一条 "hello esp32-s3"
         * （publish 参数：主题、负载、长度 0=自动、QoS=1、retain=0） */
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_subscribe(client, CONFIG_MQTT_TOPIC, 0);
        esp_mqtt_client_publish(client, CONFIG_MQTT_TOPIC, "hello esp32-s3", 0, 1, 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        /* 断开连接：仅告警（esp-mqtt 内部会自动重连） */
        ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        /* 订阅请求被确认，msg_id 用于对应是哪一次操作 */
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        /* 发布请求被确认（QoS>0 时才有此确认） */
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        /* 收到订阅主题的消息：topic/data 不以 '\0' 结尾，用 %.*s 按长度打印 */
        ESP_LOGI(TAG, "TOPIC=%.*s DATA=%.*s",
                 event->topic_len, event->topic, event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        break;
    }
}

/**
 * @brief 连接 WiFi 并启动 MQTT 客户端
 *
 * 逻辑流程：
 *   ① 先联网（STA）拿到 IP，失败直接返回
 *   ② 组装配置：Broker 地址取菜单配置，client_id 标识本设备
 *   ③ 创建客户端句柄
 *   ④ 注册事件回调并启动客户端（异步连接 Broker，函数立即返回）
 */
esp_err_t bsp_mqtt_start(void)
{
    /* ① 先联网：MQTT 走 TCP，必须先有 IP */
    esp_err_t ret = bsp_wifi_sta_connect();
    if (ret != ESP_OK) return ret;

    /* ② 组装配置：Broker 地址取自 menuconfig，client_id 用于在 Broker 上标识本设备 */
    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = CONFIG_MQTT_BROKER_URI,
        .credentials.client_id = "esp32s3_learning",
    };
    /* ③ 创建客户端句柄（static 保存，供 get_client 复用） */
    s_client = esp_mqtt_client_init(&cfg);
    /* ④ 注册事件回调（任意事件都进同一回调），再启动客户端异步连接 Broker */
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_client);
    return ESP_OK;
}

/**
 * @brief 获取 MQTT 客户端句柄
 *
 * @return s_client，供外部调用 esp_mqtt_client_publish 等接口发布消息
 */
esp_mqtt_client_handle_t bsp_mqtt_get_client(void)
{
    /* 返回内部句柄，供示例去发布消息 */
    return s_client;
}
