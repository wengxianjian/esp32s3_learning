# 定时器学习笔记（ESPTimer 软定时器）

> 指南：第十四章 ESPTIMER实验（`ref/DNESP32S3使用指南-IDF版_V1.7.pdf`）

## 原理要点

- **定时器**：单片机内部计数器，每经历一个机器周期计数器递增，达到设定值触发中断。
- **软件定时器（ESPTimer）**：由操作系统/软件模拟，灵活（可创建大量定时器），但精度受任务调度影响。
- **硬件定时器（GPTimer）**：独立硬件计数电路，精度高、可靠性好（见第十五章）。
- ESP32-S3 系统定时器内置 2 个计数器（UNIT0/UNIT1）与 3 个比较器，用于产生定时中断。

## 关键 API（ESP-IDF `esp_timer`）

| 函数 | 作用 |
|------|------|
| `esp_timer_create()` | 创建定时器（含回调、分发方式、名称） |
| `esp_timer_start_periodic()` | 启动周期性定时器 |
| `esp_timer_stop()` / `esp_timer_delete()` | 停止 / 删除 |

- `dispatch_method`：`ESP_TIMER_TASK`（在 esp_timer 任务上下文回调，可调用多数函数）或 `ESP_TIMER_ISR`（ISR 上下文，限制多但延迟低）。

## 宏设计

- `BSP_ESPTIMER_PERIOD_US`：定时周期（500ms），定义于 `board.h`。

## 验证结果

- ✅ 编译通过，烧录成功
- ✅ 定时器每 500ms 触发：日志 `timer fired 3 → 7 → 11 times`（每 2s 递增 4 次）
- ✅ LED 每 500ms 翻转

---

## GPTimer（硬件定时器）

> 指南：第十五章 GPTIMER实验

### 与 ESPTimer 的区别

- ESPTimer（第十四章）是**软件定时器**，精度受任务调度影响。
- GPTimer（第十五章）是**硬件定时器**，独立硬件计数电路，精度高、可靠性好。

### 关键 API（ESP-IDF `driver/gptimer.h`）

| 函数 | 作用 |
|------|------|
| `gptimer_new_timer()` | 创建硬件定时器（时钟源/计数方向/分辨率） |
| `gptimer_set_alarm_action()` | 设置报警值 + 自动重载 |
| `gptimer_register_event_callbacks()` | 注册报警回调 |
| `gptimer_enable()` / `gptimer_start()` | 使能 / 启动 |

- 报警回调运行在 **ISR 上下文**，需 `IRAM_ATTR`，且只做轻量操作（本工程仅计数）。

### 宏设计

- `BSP_GPTIMER_PERIOD_US`：定时周期（500ms）。

### 验证结果

- ✅ 编译通过，烧录成功
- ✅ 定时器每 500ms 报警：日志 `gptimer fired 3 → 7 → 11 times`（每 2s 递增 4 次）
- ✅ LED 每 500ms 翻转

---

## Watchdog（任务看门狗 TWDT）

> 指南：第十六章 WATCH_DOG实验

### 原理要点

- **看门狗**：监控系统运行，程序若在规定时间内没「喂狗」（重置计时），看门狗触发复位，防止程序跑飞。
- ESP32-S3 有中断看门狗（IWDT）和任务看门狗（TWDT / MWDT）。
- TWDT 监视 FreeRTOS 任务，任务需周期调用 `esp_task_wdt_reset()` 喂狗。

### 关键 API（ESP-IDF `esp_task_wdt.h`）

| 函数 | 作用 |
|------|------|
| `esp_task_wdt_init()` | 初始化 TWDT（超时时间/触发方式） |
| `esp_task_wdt_add()` | 把任务加入监视（NULL=当前任务） |
| `esp_task_wdt_reset()` | 喂狗（重置计时） |
| `esp_task_wdt_deinit()` | 反初始化 |

### 踩坑记录

- ESP-IDF 启动时**已自动初始化 TWDT**，直接再 `esp_task_wdt_init()` 会报 `ESP_ERR_INVALID_STATE`（already initialized）。需先 `esp_task_wdt_deinit()` 再按自己的配置初始化。
- `trigger_panic=false` 走 `esp_restart()` 复位在 ISR 上下文下可能不立即生效（会连续触发多次）；`trigger_panic=true` 走 panic→打印回溯→重启，更干净。

### 验证结果

- ✅ 编译通过，烧录成功
- ✅ 喂狗 5 次后停止，3s 后 `Task watchdog got triggered` + Backtrace + 复位（`rst:0xc`）
- ✅ 复位后 demo 自动重启
