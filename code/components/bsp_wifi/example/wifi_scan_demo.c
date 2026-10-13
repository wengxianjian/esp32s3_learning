/**
 * @file wifi_scan_demo.c
 * @brief WiFi 扫描实验示例（对应指南第四十八章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 通用初始化 WiFi（NVS + netif + 事件循环 + 驱动），创建默认 STA 网络接口；
 * 2) 切到 STA 模式并启动驱动；
 * 3) 用 esp_wifi_scan_start(NULL, true) 做一次「阻塞式」扫描，扫完才继续；
 * 4) 先取回 AP 数量，再动态分配记录数组读出每条 AP 信息；
 * 5) 逐条打印 SSID / RSSI（信号强度）/ auth（加密方式），最后释放内存。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "WiFi scan"（对应宏 CONFIG_DEMO_WIFI_SCAN），
 * app_main() 就会调用 bsp_wifi_scan_demo()。
 * 该示例只扫描不连接，无需改 SSID/密码；串口应打印周边 AP 列表。
 */

#include "bsp_wifi.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_log.h"
#include <stdlib.h>

/* 日志标签：本示例日志显示为 "wifi_scan_demo" */
static const char *TAG = "wifi_scan_demo";

void bsp_wifi_scan_demo(void)
{
    ESP_LOGI(TAG, "WiFi scan demo start");
    /* 通用初始化（NVS + netif + 事件循环 + 驱动）；失败会断言并复位 */
    ESP_ERROR_CHECK(bsp_wifi_init());

    /* 创建默认 STA 网络接口（lwIP 侧）；扫描虽不连接，也需先有此接口 */
    esp_netif_create_default_wifi_sta();
    /* 切换到 STA 模式 */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    /* 启动 WiFi 驱动 */
    ESP_ERROR_CHECK(esp_wifi_start());

    /* 阻塞式扫描：第 1 参数 NULL=用当前配置扫描全部信道；
     * 第 2 参数 true=扫描完成后才返回（同步等待，方便紧跟着取结果） */
    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));

    /* 取回本次扫描到的 AP 个数 */
    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);

    if (ap_count > 0) {
        /* 按 AP 数量在堆上动态分配记录数组（每条含 SSID / RSSI / 信道等） */
        wifi_ap_record_t *ap = calloc(ap_count, sizeof(wifi_ap_record_t));
        /* number 既是入参（要读几条）也是出参（实际读到几条），此处先初始化为总数 */
        uint16_t number = ap_count;
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap));
        /* 遍历打印每个 AP：ssid=名称，rssi=信号强度(dBm，越接近 0 越强)，
         * authmode=加密方式枚举（0=OPEN，其余为 WEP/WPA/WPA2/WPA3 等） */
        for (int i = 0; i < ap_count; i++) {
            ESP_LOGI(TAG, "[%d] SSID=%s RSSI=%d auth=%d",
                     i, ap[i].ssid, ap[i].rssi, ap[i].authmode);
        }
        /* 记录数组是堆内存，用完必须释放，避免内存泄漏 */
        free(ap);
    }
    ESP_LOGI(TAG, "WiFi scan done");
}
