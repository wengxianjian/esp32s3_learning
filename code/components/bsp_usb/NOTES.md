# USB MSC（模拟 U 盘）学习笔记

## 硬件
- USB OTG（D-=IO19、D+=IO20）。
- Flash FAT 分区（fatfs 分区，1MB）。

## 原理要点
- 用 TinyUSB（esp_tinyusb 组件）的 MSC（Mass Storage Class）把设备枚举成 U 盘。
- `esp_partition_find_first` 找 FAT 分区 → `wl_mount` 磨损均衡挂载 → `tinyusb_msc_new_storage_spiflash` 注册为存储。
- `tinyusb_driver_install` + MSC 描述符安装 USB 驱动。
- 挂载点 `TINYUSB_MSC_STORAGE_MOUNT_USB` 表示存储暴露给 PC（APP 与 PC 不能同时访问）。

## 宏设计
- FAT 分区在 `partitions.csv`（fatfs 分区）。

## 验证结果

- ✅ 编译烧录成功，TinyUSB MSC 初始化成功
- ✅ USB PHY 被 OTG 接管（usbmodem 断开 = MSC 生效，ESP32-S3 的 USB-Serial-JTAG 与 OTG 共享 PHY）
- 待验证：接 USB OTG 线到 PC，看是否出现 U 盘

## 踩坑记录

- `CONFIG_TINYUSB_MSC_BUFSIZE` 默认 512，需 ≥ 磨损均衡扇区大小 4096，否则报 `TinyUSB buffer size must be at least Wear Levelling sector size`。已设为 4096。
- 需 `CONFIG_TINYUSB_MSC_ENABLED=y` 开启 MSC 类（否则 `MSC_SUBCLASS_SCSI` 未定义）。
