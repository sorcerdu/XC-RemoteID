/*
 * 模拟飞行数据实现
 */
#ifdef MOCK_DATA

#include "mock_data.h"
#include <Arduino.h>
#include <math.h>

// 圆心：上海青浦淀山湖 (31.0833° N, 120.9167° E)
static const double CENTER_LAT =  31.0833;
static const double CENTER_LON = 120.9167;
static const float  RADIUS_M   = 50.0f;
static const float  ALT_GEO    = 54.0f;    // 海拔约4m + 飞行高度50m
static const float  SPEED_MS   = 5.0f;

static const double LAT_PER_M  = 1.0 / 111320.0;
static const double LON_PER_M  = 1.0 / (111320.0 * cos(CENTER_LAT * M_PI / 180.0));

float    MockData::_angle_deg = 0.0f;
uint32_t MockData::_last_ms   = 0;

void MockData::init(RIDData &data)
{
    // 身份信息（测试用，实际由 NVS 参数覆盖）
    strncpy(data.uas_id,   "MOCK-TEST-001", 20);
    strncpy(data.reg_mark, "MOCK0001",       8);

    data.op_category  = GBOpCategory::OPEN;
    data.ua_class     = GBUAClass::LIGHT;
    data.coord_type   = GBCoordType::WGS84;

    // 遥控站：起飞点（天安门广场地面）
    data.gcs_pos_type = GBGCSPosType::TAKEOFF;
    data.gcs_lat      = CENTER_LAT;
    data.gcs_lon      = CENTER_LON;
    data.gcs_alt      = 4.0f;    // 上海海拔约 4m
    data.gcs_pos_valid = true;

    // 精度（GPS 正常精度）
    data.horiz_acc = GBHorizAcc::LT_10M;
    data.vert_acc  = GBVertAcc::LT_10M;
    data.spd_acc   = GBSpdAcc::LT_1MS;
    data.ts_acc    = GBTsAcc::LTE_100MS;

    data.location_valid = true;
    _last_ms = millis();
}

void MockData::update(RIDData &data)
{
    const uint32_t now_ms = millis();

    // 每 200ms 推进一步（5 Hz 更新，1Hz 广播）
    if (now_ms - _last_ms < 200) return;
    const float dt = (now_ms - _last_ms) * 0.001f;
    _last_ms = now_ms;

    // 绕圆：角速度 = speed / radius (rad/s)
    const float omega_deg = (SPEED_MS / RADIUS_M) * (180.0f / M_PI);
    _angle_deg += omega_deg * dt;
    if (_angle_deg >= 360.0f) _angle_deg -= 360.0f;

    const float rad = _angle_deg * M_PI / 180.0f;

    // 位置
    data.lat = CENTER_LAT + RADIUS_M * cos(rad) * LAT_PER_M;
    data.lon = CENTER_LON + RADIUS_M * sin(rad) * LON_PER_M;
    data.geo_alt_m  = ALT_GEO;
    data.baro_alt_m = ALT_GEO - 2.0f;   // 气压高度略低
    data.rel_alt_m  = 50.0f;             // 相对起飞点高度

    // 速度：切线方向
    // 切线角 = angle + 90°，从真北顺时针
    float track = _angle_deg + 90.0f;
    if (track >= 360.0f) track -= 360.0f;
    data.track_deg       = track;
    data.ground_speed_ms = SPEED_MS;
    data.vert_speed_ms   = 0.0f;   // 匀速平飞

    // 运行状态
    data.op_status = GBOpStatus::AIRBORNE;

    // 时间戳（Unix ms，用 ESP32 启动时间近似，实际应接 GPS 时间）
    // 2024-01-01 00:00:00 UTC = 1704067200000 ms
    data.timestamp_ms = 1704067200000ULL + now_ms;
}

#endif // MOCK_DATA
