/**
 * @file wifi_ap_demo.c
 * @brief WiFi AP（热点）实验示例（对应指南第五十章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 通用初始化 WiFi，并创建默认 AP 网络接口；
 * 2) 用 menuconfig 的 SSID/密码组装 AP 配置：密码鉴权 WPA/WPA2，最多 4 个客户端；
 * 3) 设为 AP 模式并启动，开发板随即变成一个热点供手机/电脑连接。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "WiFi AP"（对应宏 CONFIG_DEMO_WIFI_AP），
 * 并在 "WiFi Config" 里填 "WiFi AP SSID / Password"
 * （缺省 DNESP32S3-AP / 12345678），app_main() 就会调用 bsp_wifi_ap_demo()。
 * 用手机搜索该热点并输入密码连接即可；连上后串口仅打印 AP 已启动信息。
 */

#include "bsp_wifi.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_log.h"
#include <string.h>

/* 日志标签：本示例日志显示为 "wifi_ap_demo" */
static const char *TAG = "wifi_ap_demo";

void bsp_wifi_ap_demo(void)
{
    ESP_LOGI(TAG, "WiFi AP demo start");
    /* 通用初始化（NVS + netif + 事件循环 + 驱动） */
    ESP_ERROR_CHECK(bsp_wifi_init());

    /* 创建默认 AP 网络接口（自带默认网关 192.168.4.1 与 DHCP 服务） */
    esp_netif_create_default_wifi_ap();

    /* 组装 AP 配置：SSID/密码取自 menuconfig（CONFIG_WIFI_AP_*） */
    wifi_config_t cfg = { .ap = {
        .ssid = CONFIG_WIFI_AP_SSID,
        /* ssid_len 显式给出 SSID 长度（按 strlen 计算），比隐式判断更严谨 */
        .ssid_len = strlen(CONFIG_WIFI_AP_SSID),
        .password = CONFIG_WIFI_AP_PASS,
        /* 最多允许 4 个客户端同时接入 */
        .max_connection = 4,
        /* 密码鉴权方式：WPA/WPA2 混合 PSK */
        .authmode = WIFI_AUTH_WPA_WPA2_PSK,
    }};

    /* 设 AP 模式 → 下发配置 → 启动，热点生效 */
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &cfg);
    esp_wifi_start();

    ESP_LOGI(TAG, "AP started: SSID=%s", CONFIG_WIFI_AP_SSID);
}
