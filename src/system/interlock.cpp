/*
 * 起飞联锁实现（GB 46750-2025 §5.1.7）
 *
 * ARM_STATUS 通过 MAVLinkInput 发送，不在此文件重复包含 mavlink_helpers。
 */

#include "interlock.h"
#include "parameters.h"
#include "led.h"
#include "../transport/mavlink_input.h"
#include "../storage/flight_log.h"
#include <Arduino.h>
#include <string.h>

// ── 静态成员 ──────────────────────────────────────────────────────────────────
bool        Interlock::_ok                 = false;
const char *Interlock::_reason             = "Initializing";
uint32_t    Interlock::_last_arm_status_ms = 0;
bool        Interlock::_last_ble_tx_ok     = true;
bool        Interlock::_last_wifi_tx_ok    = true;

// 由 main.cpp 创建的 MAVLinkInput 实例，通过指针访问
static MAVLinkInput *s_mavlink = nullptr;

void Interlock::init()
{
    _ok     = false;
    _reason = "Not ready";
}

void Interlock::set_mavlink(MAVLinkInput *m)
{
    s_mavlink = m;
}

void Interlock::update(const RIDData &data)
{
    const char *reason = nullptr;

    if (strnlen(Parameters::get_str(PARAM_UAS_ID), 20) == 0) {
        reason = "UAS_ID not set";
    } else if (strnlen(Parameters::get_str(PARAM_REG_MARK), 8) == 0) {
        reason = "REG_MARK not set";
    } else if (s_mavlink && !s_mavlink->ble_ok()) {
        reason = "BLE init failed";
    } else if (s_mavlink && !s_mavlink->wifi_ok()) {
        reason = "WiFi init failed";
    } else if (!_last_ble_tx_ok) {
        reason = "BLE transmit failed";
    } else if (!_last_wifi_tx_ok) {
        reason = "WiFi transmit failed";
    } else if (!FlightLog::is_mounted()) {
        reason = "Storage unavailable";
    } else if (!data.location_valid) {
        reason = "No location data";
    }

    _ok     = (reason == nullptr);
    _reason = reason ? reason : "";

    Led::set(_ok ? Led::State::OK : Led::State::RID_FAIL);

    const uint32_t now = millis();
    if (now - _last_arm_status_ms >= 1000) {
        _last_arm_status_ms = now;
        if (s_mavlink) {
            s_mavlink->send_arm_status(_ok, _reason);
        }
    }
}

bool        Interlock::is_ok()       { return _ok; }
const char *Interlock::fail_reason() { return _reason; }

void Interlock::notify_tx_result(bool ble_ok, bool wifi_ok)
{
    _last_ble_tx_ok  = ble_ok;
    _last_wifi_tx_ok = wifi_ok;
}
