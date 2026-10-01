# MQTT 学习笔记（第五十五章）

## 原理要点
- MQTT 是轻量级发布/订阅协议，基于 TCP，适合物联网。
- ESP-IDF 用 `esp_mqtt_client`（`mqtt_client.h`）：`esp_mqtt_client_init` + `register_event` + `start`。
- 事件：`CONNECTED`（订阅/发布）、`DATA`（收到消息）、`PUBLISHED`/`SUBSCRIBED`（确认）。
- 默认连公共 broker（`mqtt://test.mosquitto.org`），可 menuconfig 改。

## 配置
- broker URI / topic 走 menuconfig（`MQTT_BROKER_URI`、`MQTT_TOPIC`）。

## 验证结果

（烧录后补充）
