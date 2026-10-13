/**
 * @file wifi_sta_demo.c
 * @brief WiFi STA（客户端）实验示例（对应指南第四十九章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 通用初始化 WiFi，并创建默认 STA 网络接口；
 * 2) 注册事件回调：STA_START 时发起连接，掉线自动重连，获取 IP 时置位事件组；
 * 3) 用 menuconfig 里的 SSID/密码配置为 STA 模式并启动；
 * 4) 阻塞等待「已获取 IP」，随后打印 "WiFi STA connected"。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "WiFi STA"（对应宏 CONFIG_DEMO_WIFI_STA），
 * 并在 "WiFi Config" 里填好 "WiFi STA SSID / Password"
 * （缺省 ALIENTEK / 12345678），app_main() 就会调用 bsp_wifi_sta_demo()。
 * 请改成身边 2.4GHz 路由器的名称与密码；连上后串口打印 got IP: x.x.x.x。
 */

#include "bsp_wifi.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string.h>

/* 日志标签：本示例日志显示为 "wifi_sta_demo" */
static const char *TAG = "wifi_sta_demo";
/* 事件组第 0 位：表示「STA 已连接并拿到 IP」 */
#define WIFI_CONNECTED_BIT BIT0

/* STA 连接事件组：本文件创建，回调里置位，主线程阻塞等待 */
static EventGroupHandle_t s_wifi_eg = NULL;

/**
 * @brief WiFi / IP 事件回调
 *
 * 逻辑流程：
 *   ① STA 启动（STA_START）→ 发起连接
 *   ② 掉线（STA_DISCONNECTED）→ 打印提示并重连
 *   ③ 获取到 IP（GOT_IP）→ 打印 IP 并置位事件组
 */
static void sta_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    /* ① 驱动启动完成，此时才可发起连接（否则报未启动错误） */
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    /* ② 掉线：打印提示并立即重连（自动重连策略） */
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "disconnected, retrying...");
        esp_wifi_connect();
    /* ③ 拿到 IP：连同 IP 地址一起打印，并置位通知主线程 */
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        /* data 实为 ip_event_got_ip_t*，内层 ip_info.ip 是 32 位 IP */
        ip_event_got_ip_t *evt = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "got IP: " IPSTR, IP2STR(&evt->ip_info.ip));
        xEventGroupSetBits(s_wifi_eg, WIFI_CONNECTED_BIT);
    }
}

void bsp_wifi_sta_demo(void)
{
    ESP_LOGI(TAG, "WiFi STA demo start");
    /* 通用初始化（NVS + netif + 事件循环 + 驱动） */
    ESP_ERROR_CHECK(bsp_wifi_init());

    /* 创建事件组 + 默认 STA 网络接口 */
    s_wifi_eg = xEventGroupCreate();
    esp_netif_create_default_wifi_sta();

    /* 注册回调：Wi-Fi 事件注册任意 ID，IP 事件只关心 GOT_IP */
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &sta_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &sta_event_handler, NULL);

    /* 组装 STA 配置，SSID/密码取自 menuconfig（CONFIG_WIFI_STA_*） */
    wifi_config_t cfg = { .sta = {
        .ssid = CONFIG_WIFI_STA_SSID,
        .password = CONFIG_WIFI_STA_PASS,
    }};
    ESP_LOGI(TAG, "connecting to SSID=%s", CONFIG_WIFI_STA_SSID);

    /* 设 STA 模式 → 下发配置 → 启动驱动（随后回调里自动发起连接） */
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &cfg);
    esp_wifi_start();

    /* 阻塞等待「已连接」位，直到回调置位（portMAX_DELAY 表示无限等待） */
    xEventGroupWaitBits(s_wifi_eg, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi STA connected");
}
