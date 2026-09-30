# KEY 学习笔记

> 指南：第十一章 KEY实验（`ref/DNESP32S3使用指南-IDF版_V1.7.pdf`）

## 硬件连接

- 独立按键为板载 **BOOT 键**，连接在 **IO0**（`board.h` 中 `BSP_KEY0_GPIO`）。
- 按键一端接 GND，另一端接 IO0，因此**按下时 IO0 读到低电平**（`BSP_KEY0_ACTIVE_LEVEL = 0`）。
- IO0 需配置**内部上拉**（`GPIO_PULLUP_ENABLE`），否则松开时电平悬空不确定。

## 原理要点

- **独立按键**：机械触点开关，按下时触点闭合、电路导通，MCU 检测到电平变化。
- **按键抖动**：机械按键按下/松开瞬间，触点会因机械弹跳在短时间内快速通断（约 5~20ms），若直接采样可能误判为多次按下。
- **消抖方式**：
  - 软件消抖：检测到电平变化后延时（例程用 10ms）再确认，跳过抖动期。
  - 硬件消抖：按键两端并联 RC 滤波电路。
- **连按 vs 非连按**：`bsp_key_scan(mode)` 的 mode 参数控制按住期间是否持续返回键值。

## 关键 API（ESP-IDF）

| 函数 | 作用 |
|------|------|
| `gpio_config()` | 配置引脚为输入模式并开启内部上拉 |
| `gpio_get_level()` | 读取引脚当前电平 |

## 宏设计

- `BSP_KEY0_GPIO`：按键引脚（定义于 `board.h`）。
- `BSP_KEY0_ACTIVE_LEVEL`：有效电平（0=低电平按下）。
- 驱动通过宏判断按下状态，换引脚/换有效电平只需改 `board.h`。

## 验证结果

- ✅ 编译通过，烧录成功
- ✅ KEY 初始化正常：GPIO0 配置为输入 + 内部上拉（启动日志 `GPIO[0] ... Pullup: 1`）
- ✅ demo 运行中：`Running demo: KEY`，提示按 BOOT 翻转 LED
- ✅ 已确认：按下 BOOT 键，LED 随之翻转

---

## EXIT（外部中断）

> 指南：第十二章 EXIT实验

### 与 KEY 的区别

- KEY（第十一章）用**轮询** `gpio_get_level()` 检测按键，需要不断查询，占用 CPU。
- EXIT（第十二章）用**外部中断**：按键按下触发中断，CPU 无需轮询，效率更高、响应更及时。

### 硬件

- 同一 BOOT 键（IO0），但配置为**下降沿触发**（`BSP_KEY0_INTR_TYPE = GPIO_INTR_NEGEDGE`）。

### 关键 API

| 函数 | 作用 |
|------|------|
| `gpio_install_isr_service()` | 安装 GPIO 中断服务（全局只能装一次，重复装返回 `ESP_ERR_INVALID_STATE`） |
| `gpio_isr_handler_add()` | 注册某个引脚的中断回调函数 |
| `gpio_intr_enable()` | 使能某引脚的中断 |
| `IRAM_ATTR` | 把 ISR 放到内部 RAM，减少中断响应延迟 |

### 设计要点

- **ISR 里只置标志**（`volatile bool`），LED 翻转交给任务轮询处理——中断处理要尽可能短，避免在 ISR 里做耗时/阻塞操作。
- 中断回调函数需 `IRAM_ATTR` 修饰。

### 验证结果

- ✅ 编译通过，烧录成功
- ✅ 中断已使能：`KEY interrupt enabled on GPIO 0 (falling edge)`
- ✅ demo 运行中：`Running demo: EXIT`
- ⏳ 待确认：按下 BOOT 键，LED 是否随之翻转
