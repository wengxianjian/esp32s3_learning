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

```bash
cd code
idf.py set-target esp32s3      # 首次
idf.py menuconfig              # 选择要运行的资源 demo（DNESP32S3 Learning Demo）
idf.py build flash monitor     # 编译、烧录、打开串口监视
```

## 当前进度

见 `RESOURCES.md` 索引表（学习状态列）。
