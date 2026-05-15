/*
 * MAVLink 数据输入层
 * 支持 ArduPilot / iNav（标准 MAVLink OpenDroneID 消息集）
 */
#pragma once

#include <Arduino.h>
#include "../gb46750/encoder.h"

class MAVLinkInput {
public:
    void init(int rx_pin, int tx_pin, uint32_t baudrate);
    void update();

    const RIDData &get_data() const { return _data; }

    bool has_fresh_location(uint32_t max_age_ms = 3000) const;
    bool has_fresh_system(uint32_t max_age_ms = 5000) const;

    // 供 Interlock 调用
    void send_arm_status(bool ok, const char *reason);

    // 供 Interlock 查询广播链路状态
    bool ble_ok()  const { return _ble_ok; }
    bool wifi_ok() const { return _wifi_ok; }
    void set_ble_ok(bool v)  { _ble_ok  = v; }
    void set_wifi_ok(bool v) { _wifi_ok = v; }

private:
    RIDData  _data{};
    uint32_t _last_location_ms  = 0;
    uint32_t _last_system_ms    = 0;
    uint32_t _last_hb_ms        = 0;
    uint8_t  _fc_sysid          = 0;
    // Unix 时间基准：由 SYSTEM_TIME 消息建立
    uint64_t _unix_base_us      = 0;  // 飞控 Unix 时间（µs）
    uint32_t _boot_base_ms      = 0;  // 对应的 millis() 值
    bool     _ble_ok            = false;
    bool     _wifi_ok           = false;

    void process_packet(void *msg_ptr);
    void send_heartbeat();

    static GBHorizAcc map_horiz_acc(uint8_t v);
    static GBVertAcc  map_vert_acc(uint8_t v);
    static GBSpdAcc   map_spd_acc(uint8_t v);
    static GBTsAcc    map_ts_acc(uint8_t v);
    static GBOpStatus map_status(uint8_t v);
};
