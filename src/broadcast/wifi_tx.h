/*
 * Wi-Fi 广播发送（GB 46750-2025 §6.1.2）
 * 使用 Wi-Fi Beacon Vendor IE 广播国标数据包
 * 注意：正常飞行模式下 AP 热点关闭，仅用于 RID 广播
 */
#pragma once

#include "../gb46750/encoder.h"

class WiFi_TX {
public:
    bool init();
    bool transmit(const RIDData &data);

private:
    bool    _initialised = false;
    uint8_t _mac[6];
    uint8_t _counter = 0;  // Message counter，与 BLE 链路一致
};
