/**
 * @file bsp_rtc_rng.c
 * @brief RTC / RNG / 内部温度传感器 板级驱动实现（均为 ESP32-S3 芯片内部资源）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 本文件把三类「芯片内部、无需外部器件」的资源封装成简单接口：
 *   bsp_rtc_set_time() / bsp_rtc_get_time()      —— 设置 / 读取系统（RTC）时间
 *   bsp_rng_random()   / bsp_rng_range()         —— 获取 32 位真随机数 / 区间随机数
 *   bsp_tsens_init()   / bsp_tsens_read_celsius()—— 安装并读取内部温度传感器
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * 【RTC 时间结构体与系统时间】
 *   ESP32-S3 没有独立的 RTC 读写 API，而是把 RTC 硬件维持的秒计数作为「系统时间」
 *   （Unix 时间戳，自 1970-01-01 UTC 起的秒数）暴露给 C 标准库：
 *     - 设置：先用 struct tm 填年月日时分秒，再用 mktime() 归一化并转换成 time_t
 *       秒数，最后 settimeofday() 写回系统时间。struct tm 关键字段是「距参照值的
 *       偏移」，最易写错：
 *         tm_year —— 自 1900 年起的年数（填 2024 年要写 year-1900）；
 *         tm_mon  —— 月份 0~11（0 代表一月，填 1 月要写 mon-1）；
 *         tm_mday —— 日 1~31；tm_hour/tm_min/tm_sec —— 时/分/秒。
 *     - 读取：time(NULL) 取当前 time_t，再 localtime_r() 转回 struct tm。
 *     系统时间由 RTC 硬件持续维持，掉电后需重新设置（本工程无纽扣电池）。
 * 【RNG 熵源】
 *   esp_random() 读取 ESP32-S3 内部 **硬件随机数发生器（RNG）**，熵源来自芯片内部的
 *   物理噪声，属于真随机数，**无需初始化**即可调用，返回 32 位无符号整数。
 * 【内部温度传感器】
 *   温度传感器集成在芯片内部（测量芯片结温），需先 temperature_sensor_install()
 *   安装（配置量程、时钟源），读取前 enable()、读完 disable() 以省电。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 温度传感器量程来自 board.h 的 BSP_TSENS_RANGE_MIN / BSP_TSENS_RANGE_MAX 宏；
 * RTC / RNG 为内部资源，无引脚宏。
 * 指南章节：第二十三章 RTC、第三十一章 RNG、第二十八章 内部温度传感器。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 *   1) struct tm 的 tm_year/tm_mon 是「偏移量」而非直接数值（-1900、-1），最易出错；
 *   2) bsp_rng_range() 用取模实现，区间长度不是 2 的幂时分布略有偏差，教学够用；
 *   3) 温度传感器读到的是**芯片结温**（通常比环境温度略高），且须在设定量程范围内；
 *   4) 温度传感器 install 后必须 enable 才能读、读完 disable 以省电。
 */

#include "bsp_rtc_rng.h"
#include "board.h"
#include <sys/time.h>
#include "esp_random.h"
#include "driver/temperature_sensor.h"
#include "esp_log.h"

/* 日志标签：打印本组件日志时统一显示为 "bsp_rtc_rng" */
static const char *TAG = "bsp_rtc_rng";

/* ==================== RTC ==================== */

/**
 * @brief 设置系统（RTC）时间
 *
 * 逻辑流程：
 *   ① 把传进来的年月日时分秒填入 struct tm（注意 tm_year/tm_mon 是偏移量）
 *   ② mktime() 把 struct tm 归一化并转换成 Unix 时间戳 time_t
 *   ③ 用 settimeofday() 把时间戳写回系统时间（此后由 RTC 硬件维持）
 *
 * @param year 年份（如 2024，内部会减去 1900 存入 tm_year）
 * @param mon  月份 1~12（内部会减去 1 存入 tm_mon）
 * @param mday 日 1~31
 * @param hour 时 0~23
 * @param min  分 0~59
 * @param sec  秒 0~59
 * @return ESP_OK 成功；ESP_FAIL 失败（settimeofday 报错）
 */
esp_err_t bsp_rtc_set_time(int year, int mon, int mday, int hour, int min, int sec)
{
    /* ① struct tm 各字段是「相对参照值的偏移」，不是直接数值：
     *    tm_year 自 1900 年起算 -> year - 1900；
     *    tm_mon  自 0 起算（0=一月）-> mon - 1；
     *    其余字段（日/时/分/秒）与常规一致。先用 {0} 清零避免残留脏值。 */
    struct tm dt = {0};
    dt.tm_year = year - 1900;
    dt.tm_mon = mon - 1;
    dt.tm_mday = mday;
    dt.tm_hour = hour;
    dt.tm_min = min;
    dt.tm_sec = sec;

    /* ② mktime() 把 struct tm 转换为 time_t（自 1970-01-01 UTC 起的秒数），
     * 并顺便把越界字段归一化（如秒=70 会进位到分钟）。 */
    time_t t = mktime(&dt);
    /* ③ 组装 timeval：tv_sec 为秒，tv_usec 为微秒，这里微秒置 0 */
    struct timeval tv = { .tv_sec = t, .tv_usec = 0 };

    /* 把时间写回系统时间（RTC 硬件开始维持），失败返回 ESP_FAIL */
    if (settimeofday(&tv, NULL) != 0) {
        ESP_LOGE(TAG, "settimeofday failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}

/**
 * @brief 读取当前系统（RTC）时间
 *
 * 逻辑流程：
 *   ① time(NULL) 取当前 Unix 时间戳（秒）
 *   ② localtime_r() 把时间戳转换为本地时间 struct tm，写入调用者提供的 dt
 *
 * @param dt 输出参数，接收当前时间的 struct tm（字段含义同标准库：tm_year 自 1900、
 *           tm_mon 自 0）
 *
 * 使用可重入版本 _r（而非 localtime()），避免多任务下的静态缓冲区竞争。
 */
void bsp_rtc_get_time(struct tm *dt)
{
    time_t now = time(NULL);       /* ① 取当前时间戳（秒） */
    localtime_r(&now, dt);         /* ② 转为本地时间并写入 dt（线程安全版本） */
}

/* ==================== RNG ==================== */

/**
 * @brief 获取一个 32 位硬件随机数
 *
 * esp_random() 读取芯片内部硬件随机数发生器（RNG），熵源来自物理噪声，
 * 属于真随机数，**无需初始化**，可直接调用。
 *
 * @return 0 ~ 0xFFFFFFFF 的随机数
 */
uint32_t bsp_rng_random(void)
{
    return esp_random();   /* 直接返回硬件随机数 */
}

/**
 * @brief 获取指定闭区间 [min, max] 内的随机数
 *
 * 逻辑流程：
 *   ① 用 esp_random() 取 32 位随机数；
 *   ② 对区间长度 (max - min + 1) 取模，得到 0 ~ (max-min) 的偏移；
 *   ③ 加上 min 平移到目标区间。
 *
 * @param min 区间下界（含）
 * @param max 区间上界（含）
 * @return min ~ max 之间的随机整数
 *
 * @note 用取模实现，当区间长度不是 2 的幂时分布有轻微偏差，教学演示够用。
 */
int bsp_rng_range(int min, int max)
{
    /* max - min + 1 是闭区间内整数个数；取模后落在 0 ~ 个数-1，
     * 再加 min 平移到 [min, max] */
    return (int)(esp_random() % (max - min + 1)) + min;
}

/* ==================== 内部温度传感器 ==================== */

/* 温度传感器句柄：由 bsp_tsens_init() 安装，初始为 NULL 表示尚未安装 */
static temperature_sensor_handle_t s_tsens = NULL;

/**
 * @brief 安装内部温度传感器
 *
 * 逻辑流程：
 *   ① 配置测量量程（range_min / range_max）与时钟源
 *   ② temperature_sensor_install() 安装传感器，句柄存入 s_tsens
 *   ③ 失败打印错误并返回；成功打印量程
 */
esp_err_t bsp_tsens_init(void)
{
    /* 传感器配置：
     * range_min / range_max ：测量范围（BSP_TSENS_RANGE_MIN ~ MAX = -10 ~ 80 ℃），
     *                         超出范围读数不可用，需按实际需求设置；
     * clk_src               ：时钟源，TEMPERATURE_SENSOR_CLK_SRC_DEFAULT 用默认源。 */
    temperature_sensor_config_t cfg = {
        .range_min = BSP_TSENS_RANGE_MIN,
        .range_max = BSP_TSENS_RANGE_MAX,
        .clk_src = TEMPERATURE_SENSOR_CLK_SRC_DEFAULT,
    };

    /* 安装传感器：成功后 s_tsens 保存句柄，供后续 enable/get/disable 使用 */
    esp_err_t ret = temperature_sensor_install(&cfg, &s_tsens);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "temperature_sensor_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "temperature sensor installed (range %d~%d C)",
             BSP_TSENS_RANGE_MIN, BSP_TSENS_RANGE_MAX);
    return ESP_OK;
}

/**
 * @brief 读取一次芯片内部温度（摄氏度）
 *
 * 逻辑流程：
 *   ① enable()   使能传感器（读取前必须使能，平时失能省电）
 *   ② get_celsius() 读取摄氏温度写入 temp
 *   ③ disable()  读完立即失能省电
 *   ④ 任一步失败打印告警并返回 temp（失败时为初值 0.0）
 *
 * @return 芯片结温（℃）；读取失败返回 0.0
 */
float bsp_tsens_read_celsius(void)
{
    float temp = 0.0f;   /* 结果缓冲，初值 0.0，失败时原样返回 */
    esp_err_t ret = temperature_sensor_enable(s_tsens);   /* ① 使能传感器 */
    if (ret == ESP_OK) {
        ret = temperature_sensor_get_celsius(s_tsens, &temp);   /* ② 读摄氏温度 */
        temperature_sensor_disable(s_tsens);                    /* ③ 读完失能省电 */
    }
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "temperature_sensor_get_celsius failed: %s", esp_err_to_name(ret));
    }
    return temp;
}
