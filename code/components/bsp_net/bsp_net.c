/**
 * @file bsp_net.c
 * @brief lwIP Socket 板级实现（为 UDP/TCP 通信提供「先联网」封装）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 本文件对外只提供一个函数：
 *   bsp_net_wifi_connect() —— 把「先连 WiFi 拿到 IP」这一步封装起来
 * UDP / TCP Client / TCP Server 三个示例都先调用它，之后才能建 socket。
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * socket 通信的前提是本机已有 IP。这里直接复用 bsp_wifi_sta_connect()：
 *   连接路由器（STA）→ 阻塞直到 DHCP 拿到 IP → 返回；
 * 返回后示例即可安全地调用 socket() / bind() / connect() / listen() 等。
 * 真正的 UDP/TCP 收发逻辑写在各自 example 目录下的 .c 文件里。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * WiFi 参数来自 menuconfig（CONFIG_WIFI_STA_SSID / PASS）；
 * socket 目标 IP 与端口来自 menuconfig（CONFIG_SOCKET_REMOTE_IP / CONFIG_SOCKET_PORT）。
 * 本文件不写死任何数字。指南章节：第五十二~五十四章。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * ① 必须联网成功后才能建 socket，否则本机 IP 无效、收发会失败。
 * ② 本函数阻塞直到拿到 IP，放在 app_main 早期调用即可，无需额外等待。
 */

#include "bsp_net.h"
#include "bsp_wifi.h"

/**
 * @brief 连接 WiFi（STA 模式）并返回，供 socket 示例使用
 *
 * 逻辑流程：
 *   ① 转调 bsp_wifi_sta_connect()，内部阻塞直到拿到 IP
 *   ② 直接返回其结果（ESP_OK 表示已联网、可以建 socket）
 */
esp_err_t bsp_net_wifi_connect(void)
{
    /* 复用 WiFi 组件的 STA 连接：内部完成初始化并阻塞到取得 IP */
    return bsp_wifi_sta_connect();
}
