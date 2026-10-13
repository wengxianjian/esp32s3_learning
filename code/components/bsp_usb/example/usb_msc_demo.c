/**
 * @file usb_msc_demo.c
 * @brief USB MSC 实验示例（把 Flash FAT 分区模拟成 U 盘）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 初始化 USB MSC，把内部 Flash 上的 FAT 分区暴露给 PC；
 * 2) 进入死循环保持程序运行（USB 设备需持续在线，不能被看门狗复位）。
 * 演示现象：串口打印 "Flash U-disk ready, connect to PC via USB"，
 * 用 USB OTG 线把板子连到电脑后，电脑上会出现一个 U 盘。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "USB_MSC" 这个 demo（对应宏 CONFIG_DEMO_USB_MSC），
 * app_main() 就会调用 bsp_usb_msc_demo()。
 * 硬件：需用 USB OTG 线（接芯片 D-/D+）连到电脑；
 * 注意 USB OTG 与 USB-Serial-JTAG 共享 PHY，切换后串口 USB 会断开属正常。
 */

#include "bsp_usb.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 日志标签：本示例打印的日志会显示为 "usb_msc_demo" */
static const char *TAG = "usb_msc_demo";

void bsp_usb_msc_demo(void)
{
    /* 提示示例开始 */
    ESP_LOGI(TAG, "USB MSC demo start");
    /* 初始化 USB MSC（挂载 Flash FAT 分区并安装 TinyUSB 驱动）；
     * 失败则打印错误并返回，避免带病运行 */
    if (bsp_usb_msc_init() != ESP_OK) {
        ESP_LOGE(TAG, "USB MSC init failed");
        return;
    }
    /* 提示 U 盘就绪，提示用户用 USB OTG 线连到电脑 */
    ESP_LOGI(TAG, "Flash U-disk ready, connect to PC via USB");
    /* 保持运行：USB 设备在线期间不能被无线循环外的逻辑结束；
     * 每秒让出一次 CPU（同时给空闲任务运行机会，避免触发看门狗）。 */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
