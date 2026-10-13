/**
 * @file bsp_usb.c
 * @brief USB MSC 板级驱动实现（把 Flash FAT 分区模拟成 U 盘）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 本文件对外提供 bsp_usb_msc_init()：把 ESP32-S3 内部 Flash 上的 FAT 分区
 * （fatfs 分区）通过 USB 大容量存储类（MSC）暴露给 PC，枚举成一个 U 盘。
 * 文件内还定义了 USB 设备/配置/字符串描述符（MSC 类）。
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * 借助 esp_tinyusb 组件的 MSC（Mass Storage Class）能力，把设备枚举成 U 盘：
 *   ① esp_partition_find_first() —— 在分区表里找到类型为 DATA、子类型为
 *      FAT（fatfs）的分区
 *   ② wl_mount()                 —— 先对分区做「磨损均衡」挂载，
 *      因为内部 Flash 有写入寿命，需由 wear_levelling 层抹平地址、延长寿命
 *   ③ tinyusb_msc_new_storage_spiflash() —— 把该分区注册为 USB MSC 存储后端
 *   ④ tinyusb_driver_install()   —— 安装 TinyUSB 驱动并加载描述符，
 *      此后 PC 侧即可像操作 U 盘一样读写该分区
 * 注意：TINYUSB_MSC_STORAGE_MOUNT_USB 表示存储「暴露给 USB 主机（PC）」，
 * 同一时刻设备侧 APP 与 PC 不能同时访问该分区。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * USB OTG 数据线 D-/D+（IO19/IO20）为芯片固定 USB PHY 引脚，
 * 无需在 board.h 定义；FAT 分区由工程 partitions.csv 提供，本文件不写死。
 * 指南章节：TinyUSB / USB MSC 相关实验（U 盘模拟）。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * 1. USB-Serial-JTAG 与 USB OTG 共享 PHY，切到 OTG 后串口 USB 会断开是正常现象；
 * 2. CONFIG_TINYUSB_MSC_BUFSIZE 必须 ≥ 磨损均衡扇区大小（4096），否则安装失败；
 * 3. 需 CONFIG_TINYUSB_MSC_ENABLED=y 打开 MSC 类；
 * 4. 需用另一根 USB OTG 线把设备连到 PC，才能看到 U 盘。
 */

#include "bsp_usb.h"
#include "esp_partition.h"
#include "esp_vfs_fat.h"
#include "wear_levelling.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tinyusb_msc.h"
#include "esp_log.h"

/* 日志标签：打印 USB 相关日志时统一显示为 "bsp_usb" */
static const char *TAG = "bsp_usb";

/* USB MSC 存储句柄：注册成功后可用来做挂载/卸载等操作。
 * 初始为 NULL 表示尚未初始化。 */
static tinyusb_msc_storage_handle_t s_storage = NULL;

/* ---- USB 描述符（MSC） ---- */

/* 配置描述符总长度 = 配置描述符 + MSC 接口描述符，供 TUD_CONFIG_DESCRIPTOR 使用 */
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

/* 接口编号：只有一个 MSC 接口，编号 0；ITF_NUM_TOTAL 表示接口总数（=1） */
enum { ITF_NUM_MSC = 0, ITF_NUM_TOTAL };

/* 端点编号：bit7=1 表示 IN（设备→主机），0 表示 OUT（主机→设备）。
 * EDPT_MSC_OUT=0x01 为主机写入设备的数据端点，EDPT_MSC_IN=0x81 为设备回读端点。 */
enum { EDPT_MSC_OUT = 0x01, EDPT_MSC_IN = 0x81 };

/* 设备描述符：向主机描述「这是什么设备、用哪个 USB 版本、厂商/产品 ID 等」。
 * bLength/bDescriptorType：描述符长度与类型（固定要求）。
 * bcdUSB=0x0200：符合 USB 2.0。
 * bDeviceClass/SubClass/Protocol：MISC + IAD，表示类别在接口描述符里指定（复合设备惯用）。
 * bMaxPacketSize0：端点 0 的最大包长，用 TinyUSB 配置的端点 0 尺寸。
 * idVendor/idProduct：厂商与产品 ID（0x303A 为 Espressif 的 VID）。
 * iManufacturer/iProduct/iSerialNumber：对应下面字符串描述符数组的下标。 */
static tusb_desc_device_t device_desc = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x303A,
    .idProduct = 0x4002,
    .bcdDevice = 0x100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01,
};

/* 配置描述符集合：先配置描述符，再 MSC 接口（含两个批量端点）。
 * TUD_CONFIG_DESCRIPTOR：配置编号 1、接口总数、字符串索引 0、总长度、
 *   属性（支持远程唤醒）、最大功耗 100mA。
 * TUD_MSC_DESCRIPTOR：接口号、字符串索引 0、OUT/IN 端点号、端点包长 64。 */
static uint8_t const msc_config_desc[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EDPT_MSC_OUT, EDPT_MSC_IN, 64),
};

/* 字符串描述符：下标 0 为语言 ID（0x0409 = 美式英语），其后依次是
 * 厂商、产品名、序列号，数组下标与设备描述符里的 iManufacturer 等对应。 */
static char const *string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },
    "ALIENTEK",
    "DNESP32S3 U-Disk",
    "123456",
    "ESP32S3 MSC",
};

/**
 * @brief 挂载 Flash FAT 分区并初始化为 USB 大容量存储设备（U 盘）
 *
 * 逻辑流程：
 *   ① 在分区表里查找 DATA/FAT（fatfs）分区
 *   ② 对分区做磨损均衡挂载（wl_mount）
 *   ③ 把该分区注册为 TinyUSB MSC 的 Flash 存储后端
 *   ④ 安装 TinyUSB 驱动并加载 MSC 描述符，PC 即可识别为 U 盘
 *
 * @return
 *      - ESP_OK            初始化成功，U 盘就绪
 *      - ESP_ERR_NOT_FOUND 分区表中找不到 FAT 分区
 *      - 其它              挂载/注册/安装失败，具体错误码见串口日志
 */
esp_err_t bsp_usb_msc_init(void)
{
    /* ── ① 查找 FAT 分区 ──────────────────────────────────────
     * 类型 DATA、子类型 FAT 的分区即 partitions.csv 里的 fatfs 分区；
     * 第 3 个参数为标签过滤，NULL 表示不限定标签（取找到的第一个）。 */
    const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                                           ESP_PARTITION_SUBTYPE_DATA_FAT, NULL);
    if (part == NULL) {
        ESP_LOGE(TAG, "FAT partition not found");
        return ESP_ERR_NOT_FOUND;
    }

    /* ── ② 磨损均衡挂载 ──────────────────────────────────────
     * wl_handle 必须为 static 且跨函数存活，故定义在函数内静态区。
     * 内部 Flash 有擦写寿命，wl_mount 会建立地址映射层，把写操作均匀分散，
     * 延长分区寿命。WL_INVALID_HANDLE 表示尚未挂载。 */
    static wl_handle_t wl_handle = WL_INVALID_HANDLE;
    esp_err_t ret = wl_mount(part, &wl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "wl_mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ── ③ 注册为 USB MSC 存储 ─────────────────────────────
     * mount_point = TINYUSB_MSC_STORAGE_MOUNT_USB：把存储暴露给 USB 主机（PC）。
     * fat_fs.base_path=NULL：不额外挂载到 VFS（本示例仅供 PC 访问）。
     * fat_fs.config.max_files=5：PC 侧最多同时打开 5 个文件。
     * format_flags=0：不做额外格式化策略。
     * medium.wl_handle：把刚挂载好的磨损均衡分区作为存储介质。 */
    tinyusb_msc_storage_config_t storage_cfg = {
        .mount_point = TINYUSB_MSC_STORAGE_MOUNT_USB,
        .fat_fs = {
            .base_path = NULL,
            .config.max_files = 5,
            .format_flags = 0,
        },
    };
    storage_cfg.medium.wl_handle = wl_handle;
    ret = tinyusb_msc_new_storage_spiflash(&storage_cfg, &s_storage);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "tinyusb_msc_new_storage_spiflash failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ── ④ 安装 TinyUSB 驱动 ───────────────────────────────
     * TINYUSB_DEFAULT_CONFIG() 给出默认配置，再替换描述符三件套：
     * device（设备描述符）、full_speed_config（全速配置描述符）、
     * string（字符串描述符）及其数量。安装完成后设备即可被 PC 枚举为 U 盘。 */
    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    tusb_cfg.descriptor.device = &device_desc;
    tusb_cfg.descriptor.full_speed_config = msc_config_desc;
    tusb_cfg.descriptor.string = string_desc_arr;
    /* string_count：字符串描述符数组元素个数（含语言 ID 那项） */
    tusb_cfg.descriptor.string_count = sizeof(string_desc_arr) / sizeof(string_desc_arr[0]);
    ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "tinyusb_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 初始化完成，提示 Flash 已作为 U 盘暴露 */
    ESP_LOGI(TAG, "USB MSC init OK, Flash exposed as U-disk");
    return ESP_OK;
}
