/**
 * @file bsp_timer.c
 * @brief 定时器板级驱动实现（ESPTimer 软定时器 + GPTimer 硬定时器 + 任务看门狗 TWDT）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 本文件对外提供三组函数：
 *   bsp_esptimer_init() / bsp_esptimer_get_tick_count()
 *       —— 创建周期软定时器，回调中翻转 LED 并累加触发次数；
 *   bsp_gptimer_init() / bsp_gptimer_get_tick_count()
 *       —— 创建周期硬定时器，报警回调（ISR）中累加报警次数；
 *   bsp_wdt_init() / bsp_wdt_feed()
 *       —— 初始化任务看门狗并把当前任务纳入监视，提供喂狗接口。
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * 【ESPTimer 与 GPTimer 的区别与适用场景】
 *   ESPTimer 是「软件定时器」：由 esp_timer 组件基于系统时钟（esp_timer_get_time）
 *   在软件层维护定时器链表，到期后由 esp_timer 任务或中断回调触发。可创建很多个
 *   （数量受内存限制），使用灵活、配置简单，但触发时刻会受任务调度与 CPU 负载影响，
 *   精度和抖动不如硬件定时器，适合 LED 闪烁、超时判断、软件周期任务等场合。
 *   GPTimer 是「硬件定时器」：由芯片内部独立的通用定时器外设计数，达到报警值由
 *   硬件直接产生中断，不占用 CPU 去维护计时，精度高、抖动小、可靠性好，适合需要
 *   精确定时/精确脉宽的场合，但定时器数量有限。本工程各用一个 500ms 周期做对比。
 *
 * 【回调运行在哪个上下文】
 *   ESPTimer 回调的上下文由 args.dispatch_method 决定：
 *     - ESP_TIMER_TASK：在 esp_timer 任务（FreeRTOS 任务）上下文执行，可调用多数
 *       非 ISR 限制的 API，但本质是任务上下文，**不能阻塞**；
 *     - ESP_TIMER_ISR：在中断上下文执行，延迟更低但限制多。
 *   本工程用 ESP_TIMER_TASK，回调里只做「计数 + 翻转 LED」这类轻量操作。
 *   GPTimer 的报警回调 on_alarm 运行在 **ISR 上下文**，因此必须加 IRAM_ATTR
 *   （把函数放进 IRAM，保证中断期间可执行），且只能做极轻量、不可阻塞的操作，
 *   本工程仅做 s_gptimer_tick++ 计数。
 *
 * 【看门狗喂狗机制】
 *   TWDT（Task Watchdog Timer，任务看门狗）监控被加入监视的任务是否按时「喂狗」。
 *   被监视任务必须周期调用 esp_task_wdt_reset() 重置计时；若超过 timeout 仍未喂狗，
 *   看门狗判定程序跑飞，按 trigger_panic 触发 panic（打印回溯）并复位，让系统自动
 *   恢复。注意 ESP-IDF 启动时可能已自动初始化 TWDT，故本文件先 esp_task_wdt_deinit()
 *   再按本工程配置 esp_task_wdt_init()。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 定时周期 / 看门狗超时均来自 board.h 的 BSP_ESPTIMER_PERIOD_US、
 * BSP_GPTIMER_PERIOD_US、BSP_WDT_TIMEOUT_MS 宏，本文件不写死数字。
 * 指南章节：第十四章 ESPTIMER实验、第十五章 GPTIMER实验、第十六章 WATCH_DOG实验。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 *   1) 回调（无论任务还是 ISR 上下文）内**不可阻塞**：不能 vTaskDelay、不能做长耗时
 *      操作，否则会拖慢甚至卡死定时器任务 / 中断；
 *   2) TWDT 已在初始化状态下再 init 会返回 ESP_ERR_INVALID_STATE，故先 deinit；
 *   3) 看门狗一旦被触发，板子会复位 —— 这是本工程 WDT 示例刻意演示的现象；
 *   4) gptimer 回调只计数、不翻转 LED：翻转 LED 涉及 GPIO 驱动，放到任务里做更安全。
 */

#include "bsp_timer.h"
#include "board.h"
#include "bsp_led.h"
#include "esp_timer.h"
#include "driver/gptimer.h"
#include "esp_task_wdt.h"
#include "esp_log.h"

/* 日志标签：打印定时器相关日志时统一显示为 "bsp_timer" */
static const char *TAG = "bsp_timer";
/* ESPTimer 句柄：由 bsp_esptimer_init() 创建，初始为 NULL 表示尚未创建 */
static esp_timer_handle_t s_timer = NULL;
/* ESPTimer 触发次数：在回调中累加，故声明为 volatile 防止编译器优化掉读取 */
static volatile uint32_t s_tick_count = 0;

/**
 * @brief ESPTimer 周期回调：累加计数并翻转 LED
 *
 * 该回调在 esp_timer 任务上下文执行（dispatch_method = ESP_TIMER_TASK），
 * 因此可安全调用 GPIO / 日志等 API，但**绝不能阻塞**，否则会拖慢整个定时器任务，
 * 导致后续定时全部延迟。
 */
static void esp_timer_cb(void *arg)
{
    (void)arg;          /* 本回调不使用 arg 参数，显式忽略以避免编译告警 */
    s_tick_count++;     /* 记录触发次数，供 bsp_esptimer_get_tick_count() 查询 */
    bsp_led_toggle();   /* 每到一次周期就翻转一次 LED，从闪烁节奏即可看出定时周期 */
}

/**
 * @brief 创建并启动周期性 ESPTimer（软件定时器）
 *
 * 逻辑流程：
 *   ① 用 create_args 描述回调、参数、分发方式、名称，调用 esp_timer_create() 创建
 *   ② 调用 esp_timer_start_periodic() 以 BSP_ESPTIMER_PERIOD_US 为周期启动
 *   ③ 任一步失败打印错误并返回对应错误码
 */
esp_err_t bsp_esptimer_init(void)
{
    /* ① 描述定时器属性：
     * callback        ：周期到期时被调用的函数（esp_timer_cb）；
     * arg             ：传给回调的用户参数，本例不需要，置 NULL；
     * dispatch_method ：回调在哪个上下文执行。ESP_TIMER_TASK 表示在 esp_timer
     *                   任务上下文执行（限制少，推荐）；另一种 ESP_TIMER_ISR
     *                   在中断上下文执行、延迟低但受限多；
     * name            ：定时器名字，仅用于调试（esp_timer_dump 时显示）。 */
    esp_timer_create_args_t args = {
        .callback = esp_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,   /* 在 esp_timer 任务上下文执行回调 */
        .name = "led_timer",
    };

    /* 创建定时器，成功后句柄写入 s_timer，后续启动/停止都依赖它 */
    esp_err_t ret = esp_timer_create(&args, &s_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_timer_create failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ② 以周期模式启动：每 BSP_ESPTIMER_PERIOD_US 微秒触发一次回调，
     * BSP_ESPTIMER_PERIOD_US = 500000us = 500ms（定义在 board.h） */
    ret = esp_timer_start_periodic(s_timer, BSP_ESPTIMER_PERIOD_US);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_timer_start_periodic failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ③ 启动成功，打印周期（宏单位是 us，除以 1000 换算成 ms 便于阅读） */
    ESP_LOGI(TAG, "ESPTimer started, period = %d ms", BSP_ESPTIMER_PERIOD_US / 1000);
    return ESP_OK;
}

/**
 * @brief 查询 ESPTimer 自启动以来累计触发的次数
 *
 * @return 触发次数（由回调中的 s_tick_count 累加）
 */
uint32_t bsp_esptimer_get_tick_count(void)
{
    return s_tick_count;   /* 直接返回回调累加的计数 */
}

/* ==================== GPTimer 硬件定时器 ==================== */

/* GPTimer 句柄：由 bsp_gptimer_init() 创建，初始为 NULL 表示尚未创建 */
static gptimer_handle_t s_gptimer = NULL;
/* GPTimer 报警次数：在 ISR 回调中累加，故声明为 volatile */
static volatile uint32_t s_gptimer_tick = 0;

/**
 * @brief GPTimer 报警回调（运行在 ISR 中断上下文）
 *
 * 逻辑流程：
 *   ① 忽略未使用的参数
 *   ② 计数自增（ISR 内只做最轻量的操作）
 *   ③ 返回 false 表示不需要唤醒高优先级任务
 *
 * 该回调带 IRAM_ATTR，会被放进 IRAM 以保证中断期间可执行；因为处于中断上下文，
 * 禁止调用任何可能阻塞或非 ISR 安全的 API（如日志、vTaskDelay）。
 */
static bool IRAM_ATTR gptimer_alarm_cb(gptimer_handle_t timer,
                                       const gptimer_alarm_event_data_t *edata,
                                       void *user_ctx)
{
    (void)timer; (void)edata; (void)user_ctx;   /* 三个参数本回调都不使用，显式忽略 */
    s_gptimer_tick++;   /* 仅计数，ISR 内不做过重操作 */
    return false;       /* 已在 ISR 内处理，无需唤醒高优先级任务 */
}

/**
 * @brief 创建并启动 GPTimer 硬件定时器（报警回调中计数）
 *
 * 逻辑流程：
 *   ① 用 gptimer_config_t 指定时钟源、计数方向、分辨率，创建定时器
 *   ② 设置报警值 + 自动重载，使定时器周期性报警
 *   ③ 注册报警回调
 *   ④ 使能并启动定时器
 *   ⑤ 任一步失败打印错误并返回对应错误码
 */
esp_err_t bsp_gptimer_init(void)
{
    /* ① 定时器基本配置：
     * clk_src       ：时钟源，GPTIMER_CLK_SRC_DEFAULT 使用默认时钟源（APB）；
     * direction     ：计数方向，GPTIMER_COUNT_UP 表示向上计数；
     * resolution_hz ：计数分辨率 1000000Hz = 1MHz，即 1 个计数 = 1us。
     *                 报警值 alarm_count 以「计数个数」为单位，故 500000 计数 = 500ms。 */
    gptimer_config_t cfg = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,   /* 1MHz -> 1 tick = 1us */
    };
    esp_err_t ret = gptimer_new_timer(&cfg, &s_gptimer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gptimer_new_timer failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ② 报警配置：
     * alarm_count          ：计数值达到该值时报警。BSP_GPTIMER_PERIOD_US = 500000，
     *                        配合 1MHz 分辨率正好是 500ms；
     * reload_count         ：报警后从哪个计数值重新开始，0 表示从 0 重新计数；
     * auto_reload_on_alarm ：报警后自动重载，形成周期报警（否则只报一次）。 */
    gptimer_alarm_config_t alarm = {
        .alarm_count = BSP_GPTIMER_PERIOD_US,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    ret = gptimer_set_alarm_action(s_gptimer, &alarm);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gptimer_set_alarm_action failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ③ 注册事件回调：on_alarm 即报警回调 gptimer_alarm_cb；
     * 第三个参数 user_ctx = NULL（本回调不需要用户上下文） */
    gptimer_event_callbacks_t cbs = {
        .on_alarm = gptimer_alarm_cb,
    };
    ret = gptimer_register_event_callbacks(s_gptimer, &cbs, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gptimer_register_event_callbacks failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ④ 使能定时器（分配并启动内部硬件资源） */
    ret = gptimer_enable(s_gptimer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gptimer_enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ⑤ 启动计数，此后硬件每到报警值即触发一次回调 */
    ret = gptimer_start(s_gptimer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gptimer_start failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "GPTimer started, period = %d ms", BSP_GPTIMER_PERIOD_US / 1000);
    return ESP_OK;
}

/**
 * @brief 查询 GPTimer 自启动以来累计报警的次数
 *
 * @return 报警次数（由 ISR 回调中的 s_gptimer_tick 累加）
 */
uint32_t bsp_gptimer_get_tick_count(void)
{
    return s_gptimer_tick;   /* 直接返回 ISR 累加的计数 */
}

/* ==================== 任务看门狗（TWDT） ==================== */

/**
 * @brief 初始化任务看门狗（TWDT）并把当前任务加入监视
 *
 * 逻辑流程：
 *   ① 先反初始化（ESP-IDF 启动时可能已自动初始化 TWDT，直接再 init 会报错）
 *   ② 用 timeout_ms / idle_core_mask / trigger_panic 配置并初始化 TWDT
 *   ③ 把当前任务加入监视（esp_task_wdt_add(NULL)）
 *   ④ 任一步失败打印错误并返回对应错误码
 */
esp_err_t bsp_wdt_init(void)
{
    /* ① TWDT 在系统启动时会被自动初始化，先反初始化再按本工程配置初始化，
     *    否则 esp_task_wdt_init() 会因「已初始化」返回 ESP_ERR_INVALID_STATE */
    esp_task_wdt_deinit();

    /* ② 看门狗配置：
     * timeout_ms     ：喂狗超时时间，BSP_WDT_TIMEOUT_MS = 3000（3s），超时未喂狗即触发；
     * idle_core_mask ：要监视的 idle 任务所在核掩码，0 表示不监视 idle 任务
     *                  （避免系统空闲时误触发）；
     * trigger_panic  ：true = 超时后触发 panic（打印回溯信息后复位），
     *                  比 false（直接 esp_restart）更利于定位问题。 */
    esp_task_wdt_config_t cfg = {
        .timeout_ms = BSP_WDT_TIMEOUT_MS,
        .idle_core_mask = 0,
        .trigger_panic = true,    /* 超时触发 panic（打印回溯后重启） */
    };

    esp_err_t ret = esp_task_wdt_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_task_wdt_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ③ 把「当前任务」加入看门狗监视（参数 NULL 表示当前任务）。
     * 加入后该任务必须周期性调用 esp_task_wdt_reset() 喂狗，否则超时复位。 */
    ret = esp_task_wdt_add(NULL);   /* 把当前任务加入看门狗监视 */
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_task_wdt_add failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "TWDT started, timeout = %d ms", BSP_WDT_TIMEOUT_MS);
    return ESP_OK;
}

/**
 * @brief 喂狗：重置任务看门狗的计时
 *
 * 被监视的任务需在超时时间内周期性调用本函数，否则看门狗超时触发复位。
 */
void bsp_wdt_feed(void)
{
    esp_task_wdt_reset();   /* 重置看门狗计时（喂狗） */
}
