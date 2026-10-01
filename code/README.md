# DNESP32S3 板载资源学习工程

基于 ESP-IDF v5.3 的单工程多组件学习项目，目标是把正点原子 **DNESP32S3** 开发板上的硬件资源逐个吃透。

## 目录结构

```
code/
├── CMakeLists.txt            # 顶层 CMake（ESP-IDF 工程）
├── sdkconfig.defaults        # 默认配置（目标芯片 esp32s3）
├── RESOURCES.md              # 资源索引映射表（资源 ↔ 宏 ↔ 组件 ↔ 章节 ↔ 状态）
├── main/                     # 统一入口（menuconfig 切换要运行的资源 demo）
│   ├── CMakeLists.txt
│   ├── Kconfig               # demo 选择菜单
│   └── app_main.c
└── components/               # 每个资源一个组件
    ├── bsp_common/           # 公共板级支持（board.h 资源宏字典）
    │   └── include/board.h
    ├── bsp_led/              # LED（GPIO 输出）
    └── ...
```

## 核心约定

1. **宏是索引主线**：所有引脚/资源信息只在 `components/bsp_common/include/board.h` 定义一次（命名 `BSP_<资源名>_<信号/属性>`），组件代码只引用宏、不写死引脚号。
2. **回溯方法**：想回顾某资源怎么用，执行 `grep -rn "BSP_<资源名>" .`，可一次性命中定义处和所有调用处。
3. **索引表**：`RESOURCES.md` 把宏前缀、组件目录、指南章节、学习状态关联起来，与 `board.h` 互相指路。

## 每个资源的学习 SOP（标准作业流程）

读资料（指南章节 + 引脚表 + 原理图）→ 写驱动 `bsp_xxx.c/.h` → 写示例 `example/xxx_demo.c` → 在 `board.h` 补充该资源宏 → 写 `NOTES.md` → 更新 `RESOURCES.md` → 编译烧录验证。

## 如何运行

本开发板有**两路 USB**：CH340 串口（`/dev/cu.usbserial-*`）与 **ESP32-S3 原生 USB-Serial-JTAG**（`/dev/cu.usbmodem*`）。推荐用原生 USB，**自动下载/复位，全程免手动按键**。

```bash
cd code
idf.py set-target esp32s3                # 首次
idf.py menuconfig                        # 选择要运行的资源 demo（DNESP32S3 Learning Demo）
idf.py -p /dev/cu.usbmodem* flash monitor # 编译+烧录+串口监视（免按键）
```

> 工程已默认配置：目标 `esp32s3`、16MB Flash（DIO）、`CONFIG_ESPTOOLPY_BEFORE_USB_RESET`、控制台走 USB-Serial-JTAG。详见 `components/bsp_led/NOTES.md`。

## 当前进度

- **M0 环境跑通** ✅：工程骨架 + LED（编译/烧录/验证链路）
- **M1 基础外设** ✅：按键/中断/串口/软硬定时器/看门狗/软硬 PWM/ADC/RTC/RNG/内部温度
- **M2 通信总线与器件群** ✅：I2C(XL9555/EEPROM/OLED/AP3216C/QMA6100P)、SPI LCD、单总线(DS18B20/DHT11)、红外收发
- **M3 显示与媒体** ✅：SD 卡(SPI)、SPIFFS、I2S 音频（RGB LCD/触摸/摄像头无硬件跳过）
- **M4 USB** ✅：Flash 模拟 U 盘（SD 卡模拟 U 盘暂缓，需 SD 卡 + SDSPI MSC 适配）
- **M5 网络** ✅：WiFi（扫描/STA/AP/配网）、lwIP Socket（UDP/TCP）、MQTT

详细逐资源状态见 `RESOURCES.md`。
