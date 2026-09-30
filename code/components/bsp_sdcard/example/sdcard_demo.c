#include "bsp_sdcard.h"
#include "board.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "sdcard_demo";

void bsp_sdcard_demo(void)
{
    ESP_LOGI(TAG, "SD card demo start");
    if (bsp_sdcard_init() != ESP_OK) {
        ESP_LOGE(TAG, "SD card init failed (请确认已插入 SD 卡)");
        return;
    }

    /* 容量信息 */
    FATFS *fs;
    DWORD free_clusters;
    if (f_getfree("0:", &free_clusters, &fs) == FR_OK) {
        uint64_t total = (uint64_t)(fs->n_fatent - 2) * fs->csize * fs->ssize;
        uint64_t free = (uint64_t)free_clusters * fs->csize * fs->ssize;
        ESP_LOGI(TAG, "SD total = %.1f MB, free = %.1f MB",
                 total / 1024.0 / 1024.0, free / 1024.0 / 1024.0);
    }

    /* 写文件 */
    const char *path = BSP_SDCARD_MOUNT_POINT "/test.txt";
    const char *content = "Hello ESP32-S3 SD card!";
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "open for write failed");
        return;
    }
    fprintf(f, "%s", content);
    fclose(f);
    ESP_LOGI(TAG, "written: %s", path);

    /* 读回并校验 */
    char buf[64] = {0};
    f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "open for read failed");
        return;
    }
    fgets(buf, sizeof(buf), f);
    fclose(f);
    ESP_LOGI(TAG, "readback: %s", buf);
    if (strcmp(buf, content) == 0) {
        ESP_LOGI(TAG, "SD read/write OK");
    } else {
        ESP_LOGE(TAG, "SD read/write mismatch!");
    }
}
