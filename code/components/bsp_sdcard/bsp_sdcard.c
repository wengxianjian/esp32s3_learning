/**
 * @file bsp_sdcard.c
 * @brief SD 卡板级驱动实现（SPI2 + FAT 文件系统）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 本文件对外提供 bsp_sdcard_init()：初始化 SPI2 总线并以「SPI 模式」
 * 访问 SD 卡，最终把卡上的 FAT 文件系统挂载到 VFS 挂载点。
 * 挂载成功后，可用标准 C 文件接口（fopen/fread/fwrite）直接读写 SD 卡。
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * ESP32-S3 通过 SPI2 总线以 SDSPI 协议访问 SD 卡，初始化分三步：
 *   ① spi_bus_initialize()    —— 初始化 SPI2 总线（自动分配 DMA 通道）
 *   ② 组装 SDSPI host 与设备槽 —— host 指定用哪条 SPI 总线，slot 指定片选
 *   ③ esp_vfs_fat_sdspi_mount()—— 挂载 FAT 文件系统到挂载点
 * 挂载点（BSP_SDCARD_MOUNT_POINT）就是一个 VFS 路径，此后 open/fopen
 * 等标准文件接口都会被 VFS 路由到 SD 卡上的 FAT 分区。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * CS 引脚与挂载点来自 board.h 的 BSP_SDCARD_* 宏；SPI 的 MOSI/MISO/SCLK
 * 复用 BSP_SPI2_* 宏，本文件不写死任何引脚号。
 * 指南章节：第三十七章 SPI_SDCARD 实验。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * 1. CS（TF_CS=IO2）与红外接收复用，插 SD 卡时不要同时使用红外接收；
 * 2. 必须插入已格式化为 FAT（FAT16/FAT32）的 SD 卡，否则挂载失败；
 * 3. 本驱动 format_if_mount_failed=false，不会自动格式化，避免误删卡内数据；
 * 4. max_files 为可同时打开文件数上限，教学用 5 足够。
 */

#include "bsp_sdcard.h"
#include "board.h"
#include "driver/sdspi_host.h"
#include "driver/sdmmc_host.h"
#include "driver/spi_common.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"

/* 日志标签：打印 SD 卡相关日志时统一显示为 "bsp_sdcard" */
static const char *TAG = "bsp_sdcard";

/* SD 卡设备句柄：挂载成功后由 esp_vfs_fat_sdspi_mount() 填充，
 * 后续查询卡信息（容量、速度等）可直接使用。初始为 NULL 表示尚未挂载。 */
static sdmmc_card_t *s_card = NULL;

/**
 * @brief 初始化 SPI2 总线并挂载 SD 卡的 FAT 文件系统
 *
 * 逻辑流程：
 *   ① 配置并初始化 SPI2 总线（引脚、DMA）
 *   ② 组装 SDSPI host（用哪条总线）与设备槽（片选脚）
 *   ③ 调用 esp_vfs_fat_sdspi_mount() 挂载 FAT，得到卡句柄 s_card
 *
 * @return
 *      - ESP_OK   挂载成功
 *      - 其它     失败（总线初始化或挂载出错），具体错误码见串口日志
 *
 * @note 挂载前必须插好 SD 卡；挂载后即可用标准 C 文件接口访问。
 */
esp_err_t bsp_sdcard_init(void)
{
    /* ── ① 初始化 SPI2 总线 ────────────────────────────────────
     * mosi/miso/sclk：三根 SPI 数据/时钟线，来自 board.h（IO11/IO13/IO12）。
     * quadwp/quadhd：QSPI 用的 WP/HD 引脚，SDSPI 不使用，置 -1 表示禁用。
     * max_transfer_sz：单次传输的最大字节数，DMA 缓冲按此分配（4000 足够）。 */
    spi_bus_config_t buscfg = {
        .mosi_io_num = BSP_SPI2_MOSI_GPIO,
        .miso_io_num = BSP_SPI2_MISO_GPIO,
        .sclk_io_num = BSP_SPI2_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    /* SPI_DMA_CH_AUTO：由驱动自动选择空闲 DMA 通道，省去手动指定 */
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ── ② 组装 SDSPI host 与设备槽 ───────────────────────────
     * host：描述「用哪条 SPI 总线 + SDSPI 协议参数」，
     *       SDSPI_HOST_DEFAULT() 给出默认值，再把 slot 改成 SPI2_HOST。
     * slot：描述「SD 卡挂在总线上的哪个片选（CS）脚」，
     *       gpio_cs 来自 board.h 的 BSP_SDCARD_CS_GPIO。 */
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;

    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.host_id = SPI2_HOST;
    slot.gpio_cs = BSP_SDCARD_CS_GPIO;

    /* ── ③ 挂载参数与挂载 ─────────────────────────────────────
     * format_if_mount_failed=false：挂载失败不自动格式化，保护卡内数据。
     * max_files=5：最多同时打开 5 个文件。
     * allocation_unit_size=16*1024：FATFS 的簇分配单元，影响读写效率与空间利用。 */
    esp_vfs_fat_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    /* 挂载到 BSP_SDCARD_MOUNT_POINT，成功后卡句柄写入 s_card */
    ret = esp_vfs_fat_sdspi_mount(BSP_SDCARD_MOUNT_POINT, &host, &slot, &mount_cfg, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed: %s", esp_err_to_name(ret));
        return ret;
    }
    /* 打印实际挂载点，方便对照后续文件路径（如 /sdcard/test.txt） */
    ESP_LOGI(TAG, "SD card mounted at %s", BSP_SDCARD_MOUNT_POINT);
    return ESP_OK;
}
