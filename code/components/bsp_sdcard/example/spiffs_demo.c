#include "bsp_spiffs.h"
#include "board.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "spiffs_demo";

void bsp_spiffs_demo(void)
{
    ESP_LOGI(TAG, "SPIFFS demo start");
    ESP_ERROR_CHECK(bsp_spiffs_init());

    /* 写文件 */
    const char *path = BSP_SPIFFS_MOUNT_POINT "/test.txt";
    const char *content = "Hello SPIFFS!";
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
        ESP_LOGI(TAG, "SPIFFS read/write OK");
    } else {
        ESP_LOGE(TAG, "SPIFFS read/write mismatch!");
    }
}
