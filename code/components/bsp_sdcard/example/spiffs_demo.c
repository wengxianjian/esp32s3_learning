/**
 * @file spiffs_demo.c
 * @brief SPIFFS 实验示例（对应指南第三十八章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 挂载内部 Flash 上的 SPIFFS 文件系统（首次会自动格式化）；
 * 2) 写一个文件 test.txt，再读回并比对内容，验证「读/写一致」。
 * 演示现象：串口打印 "SPIFFS mount OK: total=..., used=..."（来自初始化）
 * 以及 "SPIFFS read/write OK"，说明内部 Flash 文件系统工作正常。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "SPIFFS" 这个 demo（对应宏 CONFIG_DEMO_SPIFFS），
 * app_main() 就会调用 bsp_spiffs_demo()。
 * 硬件：无需外部硬件，使用内部 Flash 的 spiffs 分区。
 */

#include "bsp_spiffs.h"
#include "board.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

/* 日志标签：本示例打印的日志会显示为 "spiffs_demo" */
static const char *TAG = "spiffs_demo";

void bsp_spiffs_demo(void)
{
    /* 提示示例开始 */
    ESP_LOGI(TAG, "SPIFFS demo start");
    /* 挂载 SPIFFS。ESP_ERROR_CHECK：失败会打印断言并复位，避免带病运行 */
    ESP_ERROR_CHECK(bsp_spiffs_init());

    /* ── 写文件 ────────────────────────────────────────────────
     * 路径 = 挂载点 + "/test.txt"，即 /spiffs/test.txt。
     * "w" 为写模式：文件不存在则创建，存在则清空重写。 */
    const char *path = BSP_SPIFFS_MOUNT_POINT "/test.txt";
    const char *content = "Hello SPIFFS!";
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "open for write failed");
        return;
    }
    /* 写入字符串内容，再关闭文件（关闭时把数据刷入 Flash） */
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
        ESP_LOGI(TAG, "SPIFFS read/write OK");
    } else {
        ESP_LOGE(TAG, "SPIFFS read/write mismatch!");
    }
}
