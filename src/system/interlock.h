/*
 * 起飞联锁逻辑（GB 46750-2025 §5.1.7）
 */
#pragma once

#include "../gb46750/encoder.h"

class MAVLinkInput;  // forward declaration

class Interlock {
public:
    static void init();
    static void set_mavlink(MAVLinkInput *m);
    static void update(const RIDData &data);
    // 每次广播后由 main.cpp 调用，传入实际发送结果
    static void notify_tx_result(bool ble_ok, bool wifi_ok);

    static bool        is_ok();
    static const char *fail_reason();

private:
    static bool        _ok;
    static const char *_reason;
    static uint32_t    _last_arm_status_ms;
    static bool        _last_ble_tx_ok;
    static bool        _last_wifi_tx_ok;
};
