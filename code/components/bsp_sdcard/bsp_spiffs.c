/**
 * @file bsp_spiffs.c
 * @brief SPIFFS 文件系统板级驱动实现（内部 Flash）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 本文件对外提供 bsp_spiffs_init()：把分区表中名为 "spiffs" 的数据分区
 * 注册并挂载为 SPIFFS 文件系统，同时查询该分区的总容量与已用容量。
 * 挂载成功后即可用标准 C 文件接口（fopen/fread/fwrite）读写。
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * SPIFFS 是专为嵌入式 Flash 设计的轻量文件系统，直接跑在 ESP32-S3 内部
 * Flash 的某个数据分区上，无需任何外部硬件。
 *   esp_vfs_spiffs_register() —— 注册 VFS 并挂载，挂载点即 VFS 路径
 *   esp_spiffs_info()         —— 读出该分区的 total（总字节）与 used（已用字节）
 * 挂载后 SPIFFS 成为 VFS 节点，标准文件接口会被路由到该分区。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 挂载点与分区标签来自 board.h 的 BSP_SPIFFS_MOUNT_POINT /
 * BSP_SPIFFS_PARTITION 宏，本文件不写死字符串。
 * 指南章节：第三十八章 SPIFFS 实验。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * 1. 分区表中必须有名为 "spiffs" 的数据分区（本工程 1MB），否则注册失败；
 * 2. format_if_mount_failed=true：首次挂载或分区损坏时自动格式化（会清空该分区）；
 * 3. SPIFFS 空间有限、写入寿命有限，不适合频繁改写大文件。
 */

#include "bsp_spiffs.h"
#include "board.h"
#include "esp_spiffs.h"
#include "esp_log.h"

/* 日志标签：打印 SPIFFS 相关日志时统一显示为 "bsp_spiffs" */
static const char *TAG = "bsp_spiffs";

/**
 * @brief 挂载 SPIFFS 文件系统（失败时自动格式化）并打印容量
 *
 * 逻辑流程：
 *   ① 用 board.h 宏组装 SPIFFS 配置（挂载点、分区、最大文件数、自动格式化）
 *   ② esp_vfs_spiffs_register() 注册并挂载
 *   ③ esp_spiffs_info() 查询并打印 total / used 容量
 *
 * @return
 *      - ESP_OK   挂载并查询成功
 *      - 其它     挂载失败（如无该分区），具体错误码见串口日志
 */
esp_err_t bsp_spiffs_init(void)
{
    /* ── ① 组装 SPIFFS 配置 ────────────────────────────────────
     * base_path：挂载点（VFS 路径），来自 BSP_SPIFFS_MOUNT_POINT（/spiffs）。
     * partition_label：分区标签，来自 BSP_SPIFFS_PARTITION（"spiffs"）。
     * max_files=5：最多同时打开 5 个文件。
     * format_if_mount_failed=true：挂载失败自动格式化后重试（首次烧录即如此）。 */
    esp_vfs_spiffs_conf_t conf = {
        .base_path = BSP_SPIFFS_MOUNT_POINT,
        .partition_label = BSP_SPIFFS_PARTITION,
        .max_files = 5,
        .format_if_mount_failed = true,
    };
    /* ── ② 注册并挂载 ───────────────────────────────────────── */
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ── ③ 查询容量 ────────────────────────────────────────────
     * total：分区总字节数；used：已占用字节数。查询失败不影响挂载结果。 */
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(BSP_SPIFFS_PARTITION, &total, &used);
    if (ret == ESP_OK) {
        /* 用 %d 打印容量（教学用途，容量远小于 int 上限），便于确认格式化是否生效 */
        ESP_LOGI(TAG, "SPIFFS mount OK: total=%d, used=%d", total, used);
    }
    return ret;
}
