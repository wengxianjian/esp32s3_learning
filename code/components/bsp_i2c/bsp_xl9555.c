/**
 * @file bsp_xl9555.c
 * @brief XL9555 IO 扩展芯片驱动实现（I2C0，器件地址 0x20）
 *
 * ── 整体功能 ─────────────────────────────────────────────────
 * 本文件通过 I2C0 操作 XL9555 这款 16 位 IO 扩展芯片：
 *   bsp_xl9555_init()      —— 初始化总线并把各 IO 配成输入/输出（含蜂鸣器初始关闭）
 *   bsp_xl9555_pin_write() —— 写某个 IO 的电平（高/低）
 *   bsp_xl9555_pin_read()  —— 读某个 IO 的输入电平
 * 开发板上按键（KEY0~KEY3）与中断脚（AP_INT/QMA_INT）挂在 XL9555 上，
 * 蜂鸣器也由它的 P03 控制，故上层用这三个函数即可操作这些外设。
 *
 * ── 工作原理 / 数据流 ────────────────────────────────────────
 * XL9555 相当于「I2C 版的 16 路 GPIO」，内部寄存器（本文件只用其中 6 个）：
 *   Port0 = P00~P07，Port1 = P10~P17，两者拼成 16 位引脚掩码：
 *     bit n<8 对应 P0n，bit n>=8 对应 P1(n-8)。
 *   寄存器编号：0=输入Port0、1=输入Port1、2=输出Port0、3=输出Port1、
 *              6=配置Port0、7=配置Port1。
 *   「配置寄存器」每一位含义：1=输入（高阻），0=输出。
 *   写输出：把 16 位输出值拆成低字节写 2 号寄存器、高字节写 3 号寄存器。
 *   读输入：从 0 号寄存器起连续读 2 字节，低字节是 Port0、高字节是 Port1，
 *          再拼回 16 位掩码取对应 bit。
 *
 * ── 配置来源 ─────────────────────────────────────────────────
 * 器件地址 BSP_XL9555_ADDR=0x20、蜂鸣器引脚掩码 BSP_XL9555_BEEP_IO=0x0008(P03)
 * 均来自 board.h；总线引脚/频率由 bsp_i2c.c 从 BSP_I2C0_* 取。本文件不写死地址。
 * 指南章节：第十九章 IIC_EXIO实验。
 *
 * ── 注意事项 / 踩坑 ──────────────────────────────────────────
 * 1) 必须先 bsp_xl9555_init()（内部会先初始化 I2C0）再调用读写，否则总线未就绪。
 * 2) 用 s_output/s_config 两个 static 变量在软件侧「影子」记录输出值与配置；
 *    每次改单个引脚时只改影子值里的对应位，再把整个端口写回，避免读改写竞态。
 * 3) 配置值 0xF003 含义：P00/P01（AP_INT/QMA_INT）与 P14~P17（KEY0~KEY3）
 *    设为输入，其余（含蜂鸣器 P03）设为输出。
 * 4) 初始化会顺带把蜂鸣器置低（关闭），避免上电即响。
 */

#include "bsp_i2c.h"
#include "board.h"
#include "esp_log.h"

/* 日志标签：本文件日志统一显示为 "bsp_xl9555" */
static const char *TAG = "bsp_xl9555";

/* XL9555 寄存器 */
#define XL9555_INPUT_PORT0_REG    0    /* 输入寄存器 Port0（P00~P07） */
#define XL9555_INPUT_PORT1_REG    1    /* 输入寄存器 Port1（P10~P17） */
#define XL9555_OUTPUT_PORT0_REG   2    /* 输出寄存器 Port0 */
#define XL9555_OUTPUT_PORT1_REG   3    /* 输出寄存器 Port1 */
#define XL9555_CONFIG_PORT0_REG   6    /* 方向配置寄存器 Port0（1=输入，0=输出） */
#define XL9555_CONFIG_PORT1_REG   7    /* 方向配置寄存器 Port1（1=输入，0=输出） */

/* 软件侧「影子」寄存器：s_output 记录 16 位输出电平，s_config 记录方向配置。
 * 目的是改单个引脚时在其上做位运算，再把整端口写回，避免直接读改写硬件寄存器。 */
static uint16_t s_output = 0;
static uint16_t s_config = 0;

/**
 * @brief 初始化 XL9555：先初始化 I2C0，再配置各 IO 方向并关闭蜂鸣器
 *
 * 逻辑流程：
 *   ① 初始化 I2C0 总线（失败则直接返回错误码）
 *   ② 设置方向配置掩码 s_config，并分别写入 Port0/Port1 配置寄存器
 *   ③ 把蜂鸣器（P03）初始置低，防止上电误响
 *   ④ 打印初始化成功日志
 */
esp_err_t bsp_xl9555_init(void)
{
    /* ① 确保 I2C0 已就绪（内部幂等，可安全重复调用） */
    esp_err_t ret = bsp_i2c_init();
    if (ret != ESP_OK) return ret;

    /* ② 配置：P00/P01(AP_INT/QMA_INT)、P14~P17(KEY0~KEY3) 为输入，其余为输出。
     * 0xF003 = 二进制 1111 0000 0000 0011：
     *   bit0、bit1 = 1 → P00、P01 输入；
     *   bit12~bit15 = 1 → P14~P17 输入；
     *   其余位 = 0 → 输出（含蜂鸣器 P03）。 */
    s_config = 0xF003;
    /* 写 Port0 配置寄存器：取配置值低 8 位（P00~P07 的方向） */
    ret = bsp_i2c_write_reg(BSP_XL9555_ADDR, XL9555_CONFIG_PORT0_REG,
                            (uint8_t[]){s_config & 0xFF}, 1);
    if (ret != ESP_OK) return ret;
    /* 写 Port1 配置寄存器：取配置值高 8 位（P10~P17 的方向） */
    ret = bsp_i2c_write_reg(BSP_XL9555_ADDR, XL9555_CONFIG_PORT1_REG,
                            (uint8_t[]){(s_config >> 8) & 0xFF}, 1);
    if (ret != ESP_OK) return ret;

    /* ③ 初始关闭蜂鸣器（P03 输出低电平） */
    bsp_xl9555_pin_write(BSP_XL9555_BEEP_IO, 0);

    /* ④ 打印器件地址（0x%02x → 0x20），确认初始化完成 */
    ESP_LOGI(TAG, "XL9555 init OK (addr 0x%02x)", BSP_XL9555_ADDR);
    return ESP_OK;
}

/**
 * @brief 写某个 IO 引脚的电平（通过影子寄存器 + 整端口写回实现）
 *
 * @param pin 引脚掩码：bit n<8 为 P0n，bit n>=8 为 P1(n-8)；可用 | 组合多个引脚
 * @param val 非 0 输出高电平，0 输出低电平
 *
 * 逻辑流程：
 *   ① 按 val 在影子值 s_output 中置位/清位对应引脚
 *   ② 把 16 位输出值拆成低/高字节，分别写入 Port0/Port1 输出寄存器
 */
void bsp_xl9555_pin_write(uint16_t pin, int val)
{
    if (val) {
        s_output |= pin;        /* ① 目标位置 1（输出高） */
    } else {
        s_output &= ~pin;       /* ① 目标位清 0（输出低），不影响其他引脚 */
    }
    /* ② 低字节 → Port0 输出寄存器（P00~P07） */
    bsp_i2c_write_reg(BSP_XL9555_ADDR, XL9555_OUTPUT_PORT0_REG,
                      (uint8_t[]){s_output & 0xFF}, 1);
    /* ② 高字节 → Port1 输出寄存器（P10~P17） */
    bsp_i2c_write_reg(BSP_XL9555_ADDR, XL9555_OUTPUT_PORT1_REG,
                      (uint8_t[]){(s_output >> 8) & 0xFF}, 1);
}

/**
 * @brief 读某个 IO 引脚的输入电平
 *
 * @param pin 引脚掩码（bit n<8 为 P0n，bit n>=8 为 P1(n-8)）
 * @return 1 表示该引脚为高，0 表示低（或读取失败时 data 保持 0）
 *
 * 逻辑流程：
 *   ① 从输入 Port0 寄存器起连续读 2 字节（低字节 Port0、高字节 Port1）
 *   ② 拼成 16 位输入掩码
 *   ③ 取出目标 bit 返回 0/1
 */
int bsp_xl9555_pin_read(uint16_t pin)
{
    uint8_t data[2] = {0};      /* 索引 0=Port0、1=Port1 */
    /* ① 连续读 2 字节：XL9555 输入寄存器地址自动递增 */
    bsp_i2c_read_reg(BSP_XL9555_ADDR, XL9555_INPUT_PORT0_REG, data, 2);
    /* ② 高字节在 Port1，故 (data[1] << 8) | data[0] 还原 16 位 */
    uint16_t input = (data[1] << 8) | data[0];
    /* ③ 与掩码相与：非 0 即该引脚为高 */
    return (input & pin) ? 1 : 0;
}
