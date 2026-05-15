/*
 * BLE 5.0 广播发送（GB 46750-2025 §6.1）
 * 使用 BT5 Long Range (Coded PHY S8) 广播国标数据包
 */
#pragma once

#include "../gb46750/encoder.h"

class BLE_TX {
public:
    bool init();
    bool transmit(const RIDData &data);

private:
    bool     _initialised = false;
    bool     _started     = false;
    uint8_t  _counter     = 0;

    uint8_t  _payload[256];
};
