/**
 * @file sdcard_demo.c
 * @brief SD 卡实验示例（对应指南第三十七章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 挂载 SD 卡的 FAT 文件系统；
 * 2) 用 f_getfree() 查询并打印卡的总容量与剩余容量；
 * 3) 向卡上写一个文件 test.txt，再读回并比对内容，验证「读/写一致」。
 * 演示现象：串口打印 "SD read/write OK"，说明 SD 卡读写正常。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "SDCARD" 这个 demo（对应宏 CONFIG_DEMO_SDCARD），
 * app_main() 就会调用 bsp_sdcard_demo()。
 * 硬件：需在板载卡座插入已格式化为 FAT 的 SD 卡。
 */

#include "bsp_sdcard.h"
#include "board.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>

/* 日志标签：本示例打印的日志会显示为 "sdcard_demo" */
static const char *TAG = "sdcard_demo";

void bsp_sdcard_demo(void)
{
    /* 提示示例开始 */
    ESP_LOGI(TAG, "SD card demo start");
    /* 挂载 SD 卡；失败最常见原因是没插卡或卡不是 FAT 格式，给出中文提示 */
    if (bsp_sdcard_init() != ESP_OK) {
        ESP_LOGE(TAG, "SD card init failed (请确认已插入 SD 卡)");
        return;
    }

    /* ── 容量信息 ──────────────────────────────────────────────
     * f_getfree()：FATFS 接口，查询空闲簇数并返回卷对象。
     * 容量 = 簇数 × 每簇扇区数（csize）× 每扇区字节数（ssize）。
     * 总簇数 = n_fatent - 2（FAT 表项前两项保留，需扣除）。 */
    FATFS *fs;
    DWORD free_clusters;
    if (f_getfree("0:", &free_clusters, &fs) == FR_OK) {
        /* total：总容量（字节）；free：剩余容量（字节） */
        uint64_t total = (uint64_t)(fs->n_fatent - 2) * fs->csize * fs->ssize;
        uint64_t free = (uint64_t)free_clusters * fs->csize * fs->ssize;
        /* 换算成 MB 打印，保留一位小数便于阅读 */
        ESP_LOGI(TAG, "SD total = %.1f MB, free = %.1f MB",
                 total / 1024.0 / 1024.0, free / 1024.0 / 1024.0);
    }

    /* ── 写文件 ────────────────────────────────────────────────
     * 路径 = 挂载点 + "/test.txt"，即 /sdcard/test.txt。
     * "w" 为写模式：文件不存在则创建，存在则清空重写。 */
    const char *path = BSP_SDCARD_MOUNT_POINT "/test.txt";
    const char *content = "Hello ESP32-S3 SD card!";
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "open for write failed");
        return;
    }
    /* 写入字符串内容，再关闭文件（关闭时才会把数据刷入卡） */
    fprintf(f, "%s", content);
    fclose(f);
    ESP_LOGI(TAG, "written: %s", path);

    /* ── 读回并校验 ────────────────────────────────────────────
     * buf 初始化为 0，保证即使读取失败也不会读到垃圾数据。 */
    char buf[64] = {0};
    f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "open for read failed");
        return;
    }
    /* fgets 读一行（最多 sizeof(buf)-1 字节）到 buf，自动补 '\0' */
    fgets(buf, sizeof(buf), f);
    fclose(f);
    ESP_LOGI(TAG, "readback: %s", buf);
    /* 与写入内容逐字节比较：一致说明「写进去的 = 读出来的」 */
    if (strcmp(buf, content) == 0) {
        ESP_LOGI(TAG, "SD read/write OK");
    } else {
        ESP_LOGE(TAG, "SD read/write mismatch!");
    }
}
