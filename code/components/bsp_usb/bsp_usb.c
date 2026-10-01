#include "bsp_usb.h"
#include "esp_partition.h"
#include "esp_vfs_fat.h"
#include "wear_levelling.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tinyusb_msc.h"
#include "esp_log.h"

static const char *TAG = "bsp_usb";

static tinyusb_msc_storage_handle_t s_storage = NULL;

/* ---- USB 描述符（MSC） ---- */
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

enum { ITF_NUM_MSC = 0, ITF_NUM_TOTAL };

enum { EDPT_MSC_OUT = 0x01, EDPT_MSC_IN = 0x81 };

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

static uint8_t const msc_config_desc[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EDPT_MSC_OUT, EDPT_MSC_IN, 64),
};

static char const *string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },
    "ALIENTEK",
    "DNESP32S3 U-Disk",
    "123456",
    "ESP32S3 MSC",
};

esp_err_t bsp_usb_msc_init(void)
{
    /* 1. 查找 FAT 分区 */
    const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                                           ESP_PARTITION_SUBTYPE_DATA_FAT, NULL);
    if (part == NULL) {
        ESP_LOGE(TAG, "FAT partition not found");
        return ESP_ERR_NOT_FOUND;
    }

    /* 2. 磨损均衡挂载 */
    static wl_handle_t wl_handle = WL_INVALID_HANDLE;
    esp_err_t ret = wl_mount(part, &wl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "wl_mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 3. 注册为 USB MSC 存储 */
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

    /* 4. 安装 TinyUSB 驱动 */
    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    tusb_cfg.descriptor.device = &device_desc;
    tusb_cfg.descriptor.full_speed_config = msc_config_desc;
    tusb_cfg.descriptor.string = string_desc_arr;
    tusb_cfg.descriptor.string_count = sizeof(string_desc_arr) / sizeof(string_desc_arr[0]);
    ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "tinyusb_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "USB MSC init OK, Flash exposed as U-disk");
    return ESP_OK;
}
