/*
 * 参数存储（NVS）
 * 所有用户可配置参数的读写接口
 */
#pragma once

#include <stdint.h>

class Parameters {
public:
    static void init();
    static bool is_configured();   // 是否已完成首次配置

    // 读取接口
    static const char *get_str(const char *key);
    static uint8_t     get_uint8(const char *key);
    static uint32_t    get_uint32(const char *key);

    // 写入接口
    static void set_str(const char *key, const char *val);
    static void set_uint8(const char *key, uint8_t val);
    static void set_uint32(const char *key, uint32_t val);

    // 常用参数快捷访问
    static int      get_uart_rx_pin();
    static int      get_uart_tx_pin();
    static uint32_t get_baudrate();

    // 恢复出厂默认
    static void factory_reset();

private:
    static void load_defaults();
};

// ── 参数键名常量 ──────────────────────────────────────────────────────────────
#define PARAM_UAS_ID        "UAS_ID"       // 唯一产品识别码（20字节）
#define PARAM_REG_MARK      "REG_MARK"     // 实名登记标志后8位
#define PARAM_OP_CATEGORY   "OP_CATEGORY"  // 运行类别 0~3
#define PARAM_UA_CLASS      "UA_CLASS"     // 无人机分类 0~4
#define PARAM_UART_RX       "UART_RX"      // UART RX 引脚
#define PARAM_UART_TX       "UART_TX"      // UART TX 引脚
#define PARAM_BAUDRATE      "BAUDRATE"     // 波特率
#define PARAM_WIFI_CH       "WIFI_CH"      // Wi-Fi 信道（1~13）
#define PARAM_CONFIGURED    "CONFIGURED"   // 首次配置完成标志
