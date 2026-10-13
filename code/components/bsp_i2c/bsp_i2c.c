/**
 * @file bsp_i2c.c
 * @brief I2C0 总线板级驱动实现（SDA=IO41 / SCL=IO42，400kHz 主机）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 本文件对外提供 I2C0 主机的初始化与两个「寄存器读写」原语：
 *   bsp_i2c_init()      —— 配置并安装 I2C0 主机驱动（幂等，只需调一次）
 *   bsp_i2c_write_reg() —— 向「器件地址 + 寄存器地址」写一段数据
 *   bsp_i2c_read_reg()  —— 从器件寄存器读一段数据（复合：写指针 + 重复起始读）
 * 上层 XL9555 / EEPROM / AP3216C / QMA6100P 都复用这两个原语，不再各自建链。
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * I2C 是两线（SDA 数据、SCL 时钟）同步串行总线，一次事务通用流程为：
 *   起始位 START → 7 位从机地址+读写位 → 从机 ACK → 数据字节… → 停止位 STOP。
 * ESP-IDF 用「命令链」描述事务：i2c_cmd_link_create() 建链 →
 * 逐条 append（start / write_byte / read / stop）→ i2c_master_cmd_begin()
 * 一次性提交并等待完成 → i2c_cmd_link_delete() 释放。
 *   关键点：7 位地址要「左移 1 位再或上读写位」组成 8 位首字节（写=0，读=1）。
 * 读寄存器用的是复合时序（多数 I2C 器件的通用做法）：
 *   START → 器件地址(W) → 寄存器地址 → 重复 START → 器件地址(R) → 读 N 字节 → STOP。
 * 读多字节时，前面字节主机回 ACK（告诉从机继续发），最后一个字节回 NACK
 * （告诉从机停止），随后主机发 STOP 结束；主设备必须发出这些应答位。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 引脚/频率均来自 board.h 的 BSP_I2C0_* 宏（BSP_I2C0_SDA_GPIO、
 * BSP_I2C0_SCL_GPIO、BSP_I2C0_FREQ_HZ），本文件不写死任何数字。
 * 指南章节：第十九章 IIC_EXIO实验（I2C 总线部分）。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * 1) 两个 GPIO 都使能内部上拉：I2C 总线靠上拉提供空闲高电平；
 *    若开发板已有外部上拉，这里再使能内部上拉也无害。
 * 2) 初始化是幂等的：用 s_i2c_inited 标记，重复调用直接返回，避免
 *    重复 i2c_driver_install() 触发「驱动已安装」错误。
 * 3) 每次事务超时 1000ms：器件不在线 / 地址错误会返回错误码，供上层判断。
 * 4) I2C0 由本模块独占；OLED 用的是另一条 I2C1（见 bsp_oled.c），两者互不影响。
 */

#include "bsp_i2c.h"
#include "board.h"
#include "driver/i2c.h"
#include "esp_log.h"

/* 日志标签：本文件日志统一显示为 "bsp_i2c" */
static const char *TAG = "bsp_i2c";

/* 初始化标志：true 表示 I2C0 驱动已安装，用于实现 bsp_i2c_init() 的幂等性，
 * 避免重复调用 i2c_driver_install()。初值 false 表示尚未初始化。 */
static bool s_i2c_inited = false;

/**
 * @brief 初始化 I2C0 主机总线（配置引脚/频率并安装驱动）
 *
 * 逻辑流程：
 *   ① 幂等检查：已初始化则直接返回 ESP_OK
 *   ② 用 BSP_I2C0_* 宏填充 i2c_config_t（模式/引脚/上拉/时钟）
 *   ③ i2c_param_config() 写入参数，i2c_driver_install() 安装主机驱动
 *   ④ 打印所用引脚与频率，置位初始化标志
 */
esp_err_t bsp_i2c_init(void)
{
    /* ① 幂等：若驱动已安装，直接成功返回，不重复安装 */
    if (s_i2c_inited) {
        return ESP_OK;   /* 已初始化，幂等 */
    }

    /* ② 总线参数：全部取自 board.h，不写死数字。
     * mode              ：主机模式（ESP32-S3 作为 I2C master）。
     * sda_io_num/scl_io_num：数据/时钟引脚，BSP_I2C0_SDA_GPIO=IO41、SCL=IO42。
     * sda/scl_pullup_en ：使能内部上拉，保证总线空闲时为高电平。
     * master.clk_speed  ：时钟频率 BSP_I2C0_FREQ_HZ=400kHz（快速模式）。 */
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BSP_I2C0_SDA_GPIO,
        .scl_io_num = BSP_I2C0_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = BSP_I2C0_FREQ_HZ,
    };

    /* ③ 先写入参数、再安装驱动。I2C_NUM_0 即 I2C0 控制器。
     * i2c_driver_install 后三个 0：TX/RX 缓冲长度与中断标志；
     * 主机命令链模式（本例）用默认值 0 即可。 */
    esp_err_t ret = i2c_param_config(I2C_NUM_0, &cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ④ 打印实际使用的引脚与频率，方便对照硬件接线排查问题 */
    ESP_LOGI(TAG, "I2C0 init: SDA=GPIO%d SCL=GPIO%d %d Hz",
             BSP_I2C0_SDA_GPIO, BSP_I2C0_SCL_GPIO, BSP_I2C0_FREQ_HZ);
    s_i2c_inited = true;
    return ESP_OK;
}

/**
 * @brief 向器件寄存器写数据（START → 器件地址(W) → reg → data… → STOP）
 *
 * @param addr 器件 7 位地址（内部会自动左移 1 位并拼上写位）
 * @param reg  寄存器地址（写数据前先写入的「寄存器指针」）
 * @param data 待写入的数据缓冲区；len 为 0 或 data 为 NULL 时只写寄存器地址
 * @param len  待写入的字节数
 *
 * 逻辑流程：
 *   ① 建命令链，发 START
 *   ② 发器件地址+写位、再发寄存器地址（两者均要求从机应答）
 *   ③ 若 len>0 且 data 非空，则连发 len 个数据字节
 *   ④ 发 STOP，提交事务；无论成败都释放命令链
 */
esp_err_t bsp_i2c_write_reg(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();                    /* ① 新建命令链 */
    i2c_master_start(cmd);                                          /* 起始位 START */
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);/* 7位地址左移1位 + 写位(0)，要求 ACK */
    i2c_master_write_byte(cmd, reg, true);                          /* 寄存器地址（寄存器指针） */
    if (len && data) {                                              /* ③ 有数据才写，空写仅用于定位寄存器 */
        i2c_master_write(cmd, data, len, true);                     /* 连发 len 字节，每字节检查 ACK */
    }
    i2c_master_stop(cmd);                                           /* ④ 停止位 STOP */
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000)); /* 提交执行，超时 1s */
    i2c_cmd_link_delete(cmd);                                       /* 释放命令链，避免内存泄漏 */
    return ret;
}

/**
 * @brief 从器件寄存器读数据（写指针 → 重复起始 → 读，复合时序）
 *
 * @param addr 器件 7 位地址（内部左移 1 位并拼读写位）
 * @param reg  寄存器地址（要读取的起始寄存器指针）
 * @param data 接收缓冲区
 * @param len  期望读取的字节数
 *
 * 逻辑流程：
 *   ① START → 器件地址(W) → reg：先写入要访问的寄存器指针
 *   ② 重复 START → 器件地址(R)：切换为读方向（中途不发 STOP）
 *   ③ 读 len-1 个字节（主机回 ACK，通知从机继续发），再读最后 1 个字节（主机回 NACK）
 *   ④ 发 STOP，提交；无论成败都释放命令链
 */
esp_err_t bsp_i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();                    /* ① 新建命令链 */
    i2c_master_start(cmd);                                          /* 起始位 START */
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);/* 器件地址 + 写位(0) */
    i2c_master_write_byte(cmd, reg, true);                          /* 寄存器指针 */
    i2c_master_start(cmd);                                          /* ② 重复起始（Restart），不发 STOP */
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true); /* 器件地址 + 读位(1) */
    if (len > 1) {                                                  /* ③ 多于 1 字节时，前面的字节主机回 ACK */
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);     /* 最后一个字节回 NACK，表示读结束 */
    i2c_master_stop(cmd);                                           /* ④ 停止位 STOP */
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000)); /* 提交执行，超时 1s */
    i2c_cmd_link_delete(cmd);                                       /* 释放命令链 */
    return ret;
}
