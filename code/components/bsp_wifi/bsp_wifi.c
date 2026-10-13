/**
 * @file bsp_wifi.c
 * @brief WiFi 板级驱动实现（ESP32-S3 内置 2.4GHz WiFi，STA 模式连路由器）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 本文件对外提供两个函数：
 *   bsp_wifi_init()        —— 通用初始化：NVS + netif + 事件循环 + WiFi 驱动
 *   bsp_wifi_sta_connect() —— STA 模式连接路由器，阻塞直到拿到 IP
 * 另含一个内部事件回调 sta_event_handler()，处理 Wi-Fi / IP 事件。
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * ESP-IDF 的 WiFi 采用「驱动 + 事件回调」模型，状态变化会抛出事件：
 *   WIFI_EVENT_STA_START        → 驱动已启动，此时才能发起 esp_wifi_connect()
 *   WIFI_EVENT_STA_DISCONNECTED → 掉线，这里再次 esp_wifi_connect() 实现自动重连
 *   IP_EVENT_STA_GOT_IP         → 拿到 IP，说明真正连上，置位事件组通知主线程
 * 主线程 bsp_wifi_sta_connect() 用 FreeRTOS 事件组阻塞等待「已拿到 IP」这一位，
 * 一直等到（portMAX_DELAY），从而把异步的 WiFi 连接过程封装成同步函数。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * SSID/密码来自 menuconfig：CONFIG_WIFI_STA_SSID / CONFIG_WIFI_STA_PASS
 * （对应 main/Kconfig 的 "WiFi STA SSID / Password"，缺省 ALIENTEK / 12345678），
 * 本文件不写死具体值。
 * 指南章节：第四十九章 WiFi STA 实验（本初始化被 AP / SmartConfig 示例复用）。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * ① WiFi 需要 NVS 保存校准数据，nvs_flash_init() 不能省；首次上电或分区版本
 *    不符时需 nvs_flash_erase() 后重新 init。
 * ② esp_wifi_init() 必须早于任何 esp_wifi_set_xxx() / esp_wifi_connect()。
 * ③ 此处未限制重连次数，弱信号下会持续重连；教学工程从简。
 * ④ 事件回调运行在事件循环任务中，不要在其中做长时间阻塞操作。
 */

#include "bsp_wifi.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

/* 日志标签：WiFi 日志统一显示为 "bsp_wifi" */
static const char *TAG = "bsp_wifi";

/* STA 连接事件组：用于等待「已获取 IP」。
 * 由 bsp_wifi_sta_connect() 创建，事件回调中置位，主线程阻塞等待该位。 */
static EventGroupHandle_t s_sta_eg = NULL;
/* 事件组位定义：第 0 位表示「STA 已连接并拿到 IP」 */
#define WIFI_CONNECTED_BIT BIT0

/**
 * @brief WiFi / IP 事件回调（注册到默认事件循环）
 *
 * 逻辑流程：
 *   ① STA 启动（STA_START）→ 发起连接 esp_wifi_connect()
 *   ② 连接断开（STA_DISCONNECTED）→ 自动重连 esp_wifi_connect()
 *   ③ 获取到 IP（GOT_IP）→ 打印 IP 并置位事件组，唤醒等待的主线程
 */
static void sta_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    /* ① 驱动刚启动完成：只有到这一步才允许调用 esp_wifi_connect()，
     *    否则会报 ESP_ERR_WIFI_NOT_STARTED。 */
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    /* ② 掉线事件：最简单可靠的重连策略就是再连一次（未做退避 / 次数限制） */
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    /* ③ 真正连上：DHCP 完成拿到 IP，此时 data 指向 ip_event_got_ip_t */
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        /* 把事件数据强转为 IP 事件结构体，取出其中的 IP 信息 */
        ip_event_got_ip_t *evt = (ip_event_got_ip_t *)data;
        /* IPSTR / IP2STR 是 ESP-IDF 提供的格式化宏，把 32 位 IP 拆成 a.b.c.d */
        ESP_LOGI(TAG, "got IP: " IPSTR, IP2STR(&evt->ip_info.ip));
        /* 置位「已连接」标志，唤醒在 xEventGroupWaitBits() 阻塞的主线程 */
        xEventGroupSetBits(s_sta_eg, WIFI_CONNECTED_BIT);
    }
}

/**
 * @brief 以 STA 模式连接 WiFi（阻塞直到拿到 IP）
 *
 * 逻辑流程：
 *   ① 先做通用初始化 bsp_wifi_init()，失败直接返回
 *   ② 创建事件组 + 默认 STA 网络接口
 *   ③ 注册 Wi-Fi 事件与「获取 IP」事件的回调
 *   ④ 填入 SSID/密码，设为 STA 模式并启动驱动
 *   ⑤ 阻塞等待「已获取 IP」位，等到后返回 ESP_OK
 */
esp_err_t bsp_wifi_sta_connect(void)
{
    /* ① 通用初始化（NVS / netif / 事件循环 / WiFi 驱动） */
    esp_err_t ret = bsp_wifi_init();
    if (ret != ESP_OK) return ret;

    /* ② 创建事件组用于线程同步；并创建默认的 STA 网络接口（lwIP 侧） */
    s_sta_eg = xEventGroupCreate();
    esp_netif_create_default_wifi_sta();

    /* ③ 注册事件回调：WIFI_EVENT 注册「任意事件」，IP_EVENT 只关注 GOT_IP */
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &sta_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &sta_event_handler, NULL);

    /* ④ 组装 STA 配置：SSID 与密码取自 menuconfig（CONFIG_WIFI_STA_*） */
    wifi_config_t cfg = { .sta = {
        .ssid = CONFIG_WIFI_STA_SSID,
        .password = CONFIG_WIFI_STA_PASS,
    }};
    ESP_LOGI(TAG, "connecting to SSID=%s", CONFIG_WIFI_STA_SSID);

    /* 设工作模式为 STA → 下发配置 → 启动驱动；
     * 启动后会先后触发 STA_START（回调里发起连接）与 GOT_IP（回调里置位） */
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &cfg);
    esp_wifi_start();

    /* ⑤ 阻塞等待「已连接」位：pdFALSE=等待返回后不清除标志、pdFALSE=任一位满足即
     *    返回、portMAX_DELAY=无限等待，直到回调置位。连接过程因此变成同步调用。 */
    xEventGroupWaitBits(s_sta_eg, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi STA connected");
    return ESP_OK;
}

/**
 * @brief WiFi 通用初始化：NVS + netif + 事件循环 + WiFi 驱动
 *
 * 逻辑流程：
 *   ① nvs_flash_init()：WiFi 需要 NVS 保存校准数据（失败且可恢复时先擦除重来）
 *   ② esp_netif_init() + esp_event_loop_create_default()：初始化网络接口与默认事件循环
 *   ③ esp_wifi_init()：用默认配置初始化 WiFi 驱动
 */
esp_err_t bsp_wifi_init(void)
{
    /* ① ── NVS 初始化（WiFi 校准数据存于此）───────────────────
     * 两种情况需要先擦再建：
     *   ESP_ERR_NVS_NO_FREE_PAGES     —— 无空闲页（写满）
     *   ESP_ERR_NVS_NEW_VERSION_FOUND —— 分区里是旧版本数据 */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) return ret;

    /* ② ── 网络接口 + 默认事件循环 ─────────────────────────────
     * netif 是 lwIP 的网络接口抽象；默认事件循环用于分发 Wi-Fi / IP 事件 */
    esp_netif_init();
    esp_event_loop_create_default();

    /* ③ ── WiFi 驱动初始化 ─────────────────────────────────────
     * WIFI_INIT_CONFIG_DEFAULT() 给出官方推荐默认参数（收发缓冲、任务优先级等） */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) return ret;

    ESP_LOGI(TAG, "WiFi init OK");
    return ESP_OK;
}
