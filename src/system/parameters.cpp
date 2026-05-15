/*
 * 参数存储实现（基于 ESP32 NVS）
 */

#include "parameters.h"
#include <Preferences.h>
#include <Arduino.h>

// 板型默认引脚（由 platformio.ini build_flags 注入）
#ifndef PIN_UART_RX
#define PIN_UART_RX 20
#endif
#ifndef PIN_UART_TX
#define PIN_UART_TX 21
#endif

static Preferences prefs;

void Parameters::init()
{
    prefs.begin("xc-rid", false);
    if (!prefs.isKey(PARAM_CONFIGURED)) {
        load_defaults();
    }
}

bool Parameters::is_configured()
{
    return prefs.getUChar(PARAM_CONFIGURED, 0) == 1;
}

void Parameters::load_defaults()
{
    prefs.putString(PARAM_UAS_ID,      "");
    prefs.putString(PARAM_REG_MARK,    "");
    prefs.putUChar(PARAM_OP_CATEGORY,  1);   // 开放类
    prefs.putUChar(PARAM_UA_CLASS,     1);   // 轻型
    prefs.putInt(PARAM_UART_RX,        PIN_UART_RX);
    prefs.putInt(PARAM_UART_TX,        PIN_UART_TX);
    prefs.putULong(PARAM_BAUDRATE,     115200);
    prefs.putUChar(PARAM_WIFI_CH,      6);
    // 不写 CONFIGURED，保持未配置状态
}

const char *Parameters::get_str(const char *key)
{
    static char buf[64];
    prefs.getString(key, buf, sizeof(buf));
    return buf;
}

uint8_t Parameters::get_uint8(const char *key)
{
    return prefs.getUChar(key, 0);
}

uint32_t Parameters::get_uint32(const char *key)
{
    return prefs.getULong(key, 0);
}

void Parameters::set_str(const char *key, const char *val)
{
    prefs.putString(key, val);
}

void Parameters::set_uint8(const char *key, uint8_t val)
{
    prefs.putUChar(key, val);
}

void Parameters::set_uint32(const char *key, uint32_t val)
{
    prefs.putULong(key, val);
}

int Parameters::get_uart_rx_pin()
{
    return prefs.getInt(PARAM_UART_RX, PIN_UART_RX);
}

int Parameters::get_uart_tx_pin()
{
    return prefs.getInt(PARAM_UART_TX, PIN_UART_TX);
}

uint32_t Parameters::get_baudrate()
{
    return prefs.getULong(PARAM_BAUDRATE, 115200);
}

void Parameters::factory_reset()
{
    prefs.clear();
    load_defaults();
}
