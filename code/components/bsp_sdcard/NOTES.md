# SD 卡（SPI + FAT）学习笔记

## 硬件
- SD 卡走 SPI2（MOSI=IO11、SCK=IO12、MISO=IO13），CS=TF_CS=IO2（与红外接收复用引脚）。

## 原理要点
- `spi_bus_initialize` 初始化 SPI2 总线 → `esp_vfs_fat_sdspi_mount` 挂载 FAT 文件系统。
- 挂载后 SD 卡成为 VFS 节点，可用标准 C 文件接口（fopen/fread/fwrite）。
- `f_getfree` 查询容量（总/剩余）。
- `SDSPI_HOST_DEFAULT()` / `SDSPI_DEVICE_CONFIG_DEFAULT()` 提供 SDSPI 默认配置。

## 宏设计
- `BSP_SDCARD_CS_GPIO`、`BSP_SDCARD_MOUNT_POINT`。

## 验证结果

- ⏳ 待插入 SD 卡后验证（当前卡槽无卡，挂载返回 `failed to mount card`）
