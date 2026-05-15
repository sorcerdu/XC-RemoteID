/*
 * MAVLink 数据输入层实现
 */

#include "mavlink_input.h"
#include "../system/parameters.h"
#include <Arduino.h>

// MAVLink 配置和 include（顺序敏感）
#include "mavlink_config.h"
#include <generated/mavlink_helpers.h>

// ── MAVLink 全局实例定义 ──────────────────────────────────────────────────────
mavlink_system_t mavlink_system = {0, MAV_COMP_ID_ODID_TXRX_1};

static HardwareSerial *s_serial = nullptr;

void mavlink_serial_send(mavlink_channel_t /*chan*/, const uint8_t *buf, uint8_t len)
{
    if (s_serial) s_serial->write(buf, len);
}

// ── 初始化 ────────────────────────────────────────────────────────────────────

void MAVLinkInput::init(int rx_pin, int tx_pin, uint32_t baudrate)
{
    Serial1.begin(baudrate, SERIAL_8N1, rx_pin, tx_pin);
    s_serial = &Serial1;

    _data.coord_type   = GBCoordType::WGS84;
    _data.op_category  = static_cast<GBOpCategory>(Parameters::get_uint8(PARAM_OP_CATEGORY));
    _data.ua_class     = static_cast<GBUAClass>(Parameters::get_uint8(PARAM_UA_CLASS));
    _data.gcs_pos_type = GBGCSPosType::TAKEOFF;

    strncpy(_data.uas_id,   Parameters::get_str(PARAM_UAS_ID),   20);
    _data.uas_id[20] = '\0';
    strncpy(_data.reg_mark, Parameters::get_str(PARAM_REG_MARK),  8);
    _data.reg_mark[8] = '\0';
}

// ── 主循环 ────────────────────────────────────────────────────────────────────

void MAVLinkInput::update()
{
    const uint32_t now_ms = millis();

    if (now_ms - _last_hb_ms >= 1000) {
        _last_hb_ms = now_ms;
        send_heartbeat();
    }

    mavlink_message_t msg;
    mavlink_status_t  status;
    const int avail = Serial1.available();
    for (int i = 0; i < avail; i++) {
        uint8_t c = (uint8_t)Serial1.read();
        if (mavlink_parse_char(MAVLINK_COMM_0, c, &msg, &status)) {
            process_packet(&msg);
        }
    }

    if (_last_location_ms > 0 && now_ms - _last_location_ms > 5000) {
        _data.location_valid = false;
        _data.op_status = GBOpStatus::RID_FAIL_NORMAL;
    }
}

// ── 数据包处理 ────────────────────────────────────────────────────────────────

void MAVLinkInput::process_packet(void *msg_ptr)
{
    mavlink_message_t &msg = *static_cast<mavlink_message_t *>(msg_ptr);
    const uint32_t now_ms = millis();

    switch (msg.msgid) {

    case MAVLINK_MSG_ID_HEARTBEAT: {
        mavlink_heartbeat_t hb;
        mavlink_msg_heartbeat_decode(&msg, &hb);
        if (_fc_sysid == 0 && msg.sysid > 0 && hb.type != MAV_TYPE_GCS) {
            _fc_sysid = msg.sysid;
            mavlink_system.sysid = _fc_sysid;
        }
        break;
    }

    case MAVLINK_MSG_ID_SYSTEM_TIME: {
        // 建立 Unix 时间基准，用于还原 OPEN_DRONE_ID_LOCATION.timestamp
        mavlink_system_time_t st;
        mavlink_msg_system_time_decode(&msg, &st);
        if (st.time_unix_usec > 0) {
            _unix_base_us  = st.time_unix_usec;
            _boot_base_ms  = now_ms;
        }
        break;
    }

    case MAVLINK_MSG_ID_OPEN_DRONE_ID_LOCATION: {
        mavlink_open_drone_id_location_t loc;
        mavlink_msg_open_drone_id_location_decode(&msg, &loc);

        // 未知位置：MAVLink 规定 lat==0 && lon==0 表示未知，不标记为有效
        const bool pos_known = !(loc.latitude == 0 && loc.longitude == 0);
        if (pos_known) {
            _data.lat = loc.latitude  * 1.0e-7;
            _data.lon = loc.longitude * 1.0e-7;
        }

        // 未知速度/高度：MAVLink 规定的未知值
        _data.track_deg       = (loc.direction == 36100) ? NAN : loc.direction * 0.01f;
        _data.ground_speed_ms = (loc.speed_horizontal == 25500) ? NAN : loc.speed_horizontal * 0.01f;
        _data.vert_speed_ms   = (loc.speed_vertical == 6300) ? NAN : loc.speed_vertical * 0.01f;
        _data.geo_alt_m       = (loc.altitude_geodetic   <= -999.0f) ? NAN : loc.altitude_geodetic;
        _data.baro_alt_m      = (loc.altitude_barometric <= -999.0f) ? NAN : loc.altitude_barometric;
        _data.rel_alt_m       = (loc.height <= -999.0f) ? NAN : loc.height;

        _data.horiz_acc = map_horiz_acc(loc.horizontal_accuracy);
        _data.vert_acc  = map_vert_acc(loc.vertical_accuracy);
        _data.spd_acc   = map_spd_acc(loc.speed_accuracy);
        _data.ts_acc    = map_ts_acc(loc.timestamp_accuracy);
        _data.op_status = map_status(loc.status);

        // 时间戳：loc.timestamp 是"UTC 整点后的秒数"（0~3599.999s）
        // 需要结合 SYSTEM_TIME 消息建立的 Unix 时间基准来还原完整 Unix ms
        if (_unix_base_us > 0 && loc.timestamp >= 0.0f && loc.timestamp < 3600.0f) {
            // 从基准时间推算当前 Unix 时间（ms）
            const uint32_t now_ms = millis();
            const uint64_t elapsed_ms = (now_ms >= _boot_base_ms)
                                        ? (now_ms - _boot_base_ms)
                                        : (now_ms + (0xFFFFFFFFUL - _boot_base_ms) + 1);
            const uint64_t unix_now_ms = (_unix_base_us / 1000ULL) + elapsed_ms;

            // 对齐到整点：用 loc.timestamp 替换当前时间的"整点内秒数"
            const uint64_t hour_base_ms = (unix_now_ms / 3600000ULL) * 3600000ULL;
            const uint64_t ts_ms = hour_base_ms + (uint64_t)(loc.timestamp * 1000.0f);

            // 防止跨整点时出现大跳变（±30min 容差）
            const int64_t diff = (int64_t)ts_ms - (int64_t)unix_now_ms;
            if (diff > 1800000LL) {
                _data.timestamp_ms = ts_ms - 3600000ULL;  // 上一个整点
            } else if (diff < -1800000LL) {
                _data.timestamp_ms = ts_ms + 3600000ULL;  // 下一个整点
            } else {
                _data.timestamp_ms = ts_ms;
            }
        } else {
            // 无 SYSTEM_TIME 基准，时间戳置 0（未知）
            _data.timestamp_ms = 0;
        }

        _data.location_valid = pos_known;
        // 位置未知时，除 EMERGENCY 外所有状态都覆盖为 RID_FAIL_NORMAL
        // EMERGENCY 保留原状态，让接收端知道是紧急情况而非普通 RID 失效
        if (!pos_known && _data.op_status != GBOpStatus::EMERGENCY) {
            _data.op_status = GBOpStatus::RID_FAIL_NORMAL;
        }
        _last_location_ms    = now_ms;
        break;
    }

    case MAVLINK_MSG_ID_OPEN_DRONE_ID_SYSTEM: {
        mavlink_open_drone_id_system_t sys;
        mavlink_msg_open_drone_id_system_decode(&msg, &sys);

        _data.gcs_lat = sys.operator_latitude  * 1.0e-7;
        _data.gcs_lon = sys.operator_longitude * 1.0e-7;
        _data.gcs_alt = sys.operator_altitude_geo;
        _data.gcs_pos_valid = (sys.operator_latitude != 0 || sys.operator_longitude != 0);
        _data.gcs_pos_type  = (sys.operator_location_type == MAV_ODID_OPERATOR_LOCATION_TYPE_TAKEOFF)
                              ? GBGCSPosType::TAKEOFF : GBGCSPosType::GCS;
        _last_system_ms = now_ms;
        break;
    }

    case MAVLINK_MSG_ID_OPEN_DRONE_ID_SYSTEM_UPDATE: {
        mavlink_open_drone_id_system_update_t upd;
        mavlink_msg_open_drone_id_system_update_decode(&msg, &upd);

        // 与 SYSTEM 分支保持一致：0/0 表示未知，不标记为有效
        const bool gcs_known = !(upd.operator_latitude == 0 && upd.operator_longitude == 0);
        if (gcs_known) {
            _data.gcs_lat = upd.operator_latitude  * 1.0e-7;
            _data.gcs_lon = upd.operator_longitude * 1.0e-7;
            _data.gcs_alt = upd.operator_altitude_geo;
        }
        _data.gcs_pos_valid = gcs_known;
        _last_system_ms = now_ms;
        break;
    }

    default:
        break;
    }
}

// ── 发送 ──────────────────────────────────────────────────────────────────────

void MAVLinkInput::send_heartbeat()
{
    mavlink_msg_heartbeat_send(MAVLINK_COMM_0, MAV_TYPE_ODID,
                               MAV_AUTOPILOT_INVALID, 0, 0, 0);
}

void MAVLinkInput::send_arm_status(bool ok, const char *reason)
{
    mavlink_msg_open_drone_id_arm_status_send(
        MAVLINK_COMM_0,
        ok ? MAV_ODID_ARM_STATUS_GOOD_TO_ARM
           : MAV_ODID_ARM_STATUS_PRE_ARM_FAIL_GENERIC,
        reason ? reason : "");
}

bool MAVLinkInput::has_fresh_location(uint32_t max_age_ms) const
{
    return _last_location_ms > 0 && (millis() - _last_location_ms) < max_age_ms;
}

bool MAVLinkInput::has_fresh_system(uint32_t max_age_ms) const
{
    return _last_system_ms > 0 && (millis() - _last_system_ms) < max_age_ms;
}

// ── 精度枚举映射 ──────────────────────────────────────────────────────────────

GBHorizAcc MAVLinkInput::map_horiz_acc(uint8_t v)
{
    if (v >= 12) return GBHorizAcc::LT_1M;
    if (v >= 11) return GBHorizAcc::LT_3M;
    if (v >= 10) return GBHorizAcc::LT_10M;
    if (v >= 9)  return GBHorizAcc::LT_30M;
    if (v >= 8)  return GBHorizAcc::LT_92M;
    if (v >= 7)  return GBHorizAcc::LT_185M;
    if (v >= 6)  return GBHorizAcc::LT_556M;
    if (v >= 5)  return GBHorizAcc::LT_926M;
    if (v >= 4)  return GBHorizAcc::LT_1852M;
    if (v >= 3)  return GBHorizAcc::LT_3700M;
    if (v >= 2)  return GBHorizAcc::LT_7410M;
    if (v >= 1)  return GBHorizAcc::LT_18520M;
    return GBHorizAcc::UNKNOWN_OR_GTE_18520M;
}

GBVertAcc MAVLinkInput::map_vert_acc(uint8_t v)
{
    if (v >= 6) return GBVertAcc::LT_1M;
    if (v >= 5) return GBVertAcc::LT_3M;
    if (v >= 4) return GBVertAcc::LT_10M;
    if (v >= 3) return GBVertAcc::LT_25M;
    if (v >= 2) return GBVertAcc::LT_45M;
    if (v >= 1) return GBVertAcc::LT_150M;
    return GBVertAcc::UNKNOWN_OR_GTE_150M;
}

GBSpdAcc MAVLinkInput::map_spd_acc(uint8_t v)
{
    if (v >= 4) return GBSpdAcc::LT_03MS;
    if (v >= 3) return GBSpdAcc::LT_1MS;
    if (v >= 2) return GBSpdAcc::LT_3MS;
    if (v >= 1) return GBSpdAcc::LT_10MS;
    return GBSpdAcc::UNKNOWN_OR_GTE_10MS;
}

GBTsAcc MAVLinkInput::map_ts_acc(uint8_t v)
{
    // MAV_ODID_TIME_ACC: 0=Unknown, 1=≤0.1s, 2=≤0.2s, ..., 15=≤1.5s
    // 值越小精度越高（与 horiz/vert/spd_acc 方向相同）
    switch (v) {
    case 1:  return GBTsAcc::LTE_100MS;
    case 2:  return GBTsAcc::LTE_200MS;
    case 3:  return GBTsAcc::LTE_300MS;
    case 4:  return GBTsAcc::LTE_400MS;
    case 5:  return GBTsAcc::LTE_500MS;
    // MAVLink 0.6~1.5s 均超过 500ms，映射为 UNKNOWN
    default: return GBTsAcc::UNKNOWN_OR_GT_500MS;
    }
}

GBOpStatus MAVLinkInput::map_status(uint8_t v)
{
    switch (v) {
    case MAV_ODID_STATUS_UNDECLARED:               return GBOpStatus::NOT_REPORTED;
    case MAV_ODID_STATUS_GROUND:                   return GBOpStatus::GROUND;
    case MAV_ODID_STATUS_AIRBORNE:                 return GBOpStatus::AIRBORNE;
    case MAV_ODID_STATUS_EMERGENCY:                return GBOpStatus::EMERGENCY;
    case MAV_ODID_STATUS_REMOTE_ID_SYSTEM_FAILURE: return GBOpStatus::RID_FAIL_NORMAL;
    default:                                       return GBOpStatus::NOT_REPORTED;
    }
}
