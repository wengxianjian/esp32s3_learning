/**
 * @file wifi_smartconfig_demo.c
 * @brief WiFi 一键配网 SmartConfig 实验示例（对应指南第五十一章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 通用初始化 WiFi，并创建默认 STA 网络接口；
 * 2) 注册三类事件回调：Wi-Fi 事件、获取 IP 事件、SmartConfig(SC) 事件；
 * 3) 启动 STA 后，在 STA_START 事件里调用 esp_smartconfig_start() 进入配网；
 * 4) 手机端用 ESPTouch App 广播 SSID/密码，开发板收到后自动连接；
 * 5) 阻塞等待「已连接」或「配网完成」标志，成功后打印提示。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "WiFi SmartConfig"（对应宏 CONFIG_DEMO_WIFI_SMARTCONFIG），
 * app_main() 就会调用 bsp_wifi_smartconfig_demo()。
 * 手机安装「ESP-Touch / ESPTouch」App（手机需连 2.4GHz WiFi），填好路由器 SSID
 * 与密码后一键配网；串口先打印 got SSID/password，再打印 got IP: x.x.x.x。
 */

#include "bsp_wifi.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_smartconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

/* 日志标签：本示例日志显示为 "wifi_smartconfig_demo" */
static const char *TAG = "wifi_smartconfig_demo";
/* 事件组第 0 位：已拿到 IP；第 1 位：SmartConfig 配网流程结束 */
#define SC_CONNECTED_BIT BIT0
#define SC_DONE_BIT      BIT1

/* SmartConfig 事件组：回调置位，主线程等待 */
static EventGroupHandle_t s_sc_eg = NULL;

/**
 * @brief Wi-Fi / IP / SmartConfig 事件回调
 *
 * 逻辑流程：
 *   ① STA 启动（STA_START）→ 启动 SmartConfig 配网
 *   ② 掉线（STA_DISCONNECTED）→ 打印并重连
 *   ③ 获取到 IP（GOT_IP）→ 打印 IP 并置位「已连接」
 *   ④ 收到配网信息（SC_EVENT_GOT_SSID_PSWD）→ 打印提示
 *   ⑤ 配网握手完成（SC_EVENT_SEND_ACK_DONE）→ 置位「配网完成」
 */
static void sc_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    /* ① 驱动启动完成：开始监听手机 App 的配网广播 */
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_smartconfig_start(NULL);
    /* ② 掉线：打印并重连（配网成功后又掉线也能自动恢复） */
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "disconnected, reconnect...");
        esp_wifi_connect();
    /* ③ 拿到 IP：配网成功并已连上路由 */
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *evt = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "got IP: " IPSTR, IP2STR(&evt->ip_info.ip));
        xEventGroupSetBits(s_sc_eg, SC_CONNECTED_BIT);
    /* ④ 已通过 SmartConfig 收到手机广播的 SSID/密码 */
    } else if (base == SC_EVENT && id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "got SSID/password via SmartConfig");
    /* ⑤ 配网应答发送完成，配网流程可以收尾 */
    } else if (base == SC_EVENT && id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(s_sc_eg, SC_DONE_BIT);
    }
}

void bsp_wifi_smartconfig_demo(void)
{
    ESP_LOGI(TAG, "WiFi SmartConfig demo start");
    /* 通用初始化（NVS + netif + 事件循环 + 驱动） */
    ESP_ERROR_CHECK(bsp_wifi_init());

    /* 创建事件组 + 默认 STA 网络接口 */
    s_sc_eg = xEventGroupCreate();
    esp_netif_create_default_wifi_sta();

    /* 注册回调：Wi-Fi 任意事件、获取 IP 事件、SmartConfig 任意事件 */
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &sc_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &sc_event_handler, NULL);
    esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &sc_event_handler, NULL);

    /* 配网协议类型选 ESPTouch（ESP 官方手机 App 使用的方案） */
    esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);
    /* 用默认参数构造 SmartConfig 启动配置（保留变量，便于后续扩展） */
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();

    /* 设 STA 模式 → 启动驱动；STA_START 回调里会开启 SmartConfig */
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    ESP_LOGI(TAG, "waiting for SmartConfig (use ESPTouch app)...");
    /* 等待「已连接」或「配网完成」任一位：pdTRUE=等待返回后清除这些位，
     * pdFALSE=任一位满足即返回，portMAX_DELAY=无限等待 */
    xEventGroupWaitBits(s_sc_eg, SC_CONNECTED_BIT | SC_DONE_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "SmartConfig done, WiFi connected");
}
