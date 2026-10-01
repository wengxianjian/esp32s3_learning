# WiFi 学习笔记（第四十八~五十一章）

## 硬件
- ESP32-S3 内置 2.4GHz WiFi + BT，无需外部模块。

## 原理要点
- WiFi 模式：STA（连路由）、AP（热点）、STA+AP。
- 通用初始化：`nvs_flash_init` → `esp_netif_init` → `esp_event_loop_create_default` → `esp_wifi_init`。
- 扫描：`esp_wifi_scan_start` + `esp_wifi_scan_get_ap_records`。
- STA 连接：`esp_wifi_set_config`(SSID/密码) + `esp_wifi_connect`，`IP_EVENT_STA_GOT_IP` 拿到 IP。
- AP：`esp_netif_create_default_wifi_ap` + `esp_wifi_set_config`。
- SmartConfig：`esp_smartconfig_start`（ESPTouch），手机 App 一键配网。

## 配置
- SSID/密码走 menuconfig（`WIFI STA SSID/Password`、`WIFI AP SSID/Password`）。

## 验证结果

- ✅ WiFi 扫描：`WiFi init OK`，扫到 13 个 AP（SSID/RSSI/加密方式均正常）
- ✅ WiFi AP：`AP started: SSID=DNESP32S3-AP`
- ✅ WiFi STA：连接 `ChinaNet-bFwN` 成功，`got IP: 192.168.1.22`
- 待验证：SmartConfig 配网
