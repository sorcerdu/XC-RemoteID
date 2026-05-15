/*
 * 飞行数据滚动存储实现（LittleFS）
 */

#include "flight_log.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <math.h>

uint32_t FlightLog::_total_records = 0;
uint32_t FlightLog::_file_index    = 0;
uint32_t FlightLog::_file_records  = 0;
char     FlightLog::_path[32]      = {};
bool     FlightLog::_mounted       = false;

void FlightLog::init()
{
    // 挂载 fllog 分区（LittleFS）
    // 首先尝试不格式化挂载，保护已有查证数据（§5.1.8 不可删除）
    if (!LittleFS.begin(false, "/fllog", 10, "fllog")) {
        // 挂载失败：可能是全新分区（从未格式化）或文件系统损坏
        // 尝试格式化一次（仅新设备首次使用时会走到这里）
        Serial.println("[FlightLog] mount failed, attempting format for new partition...");
        if (!LittleFS.begin(true, "/fllog", 10, "fllog")) {
            Serial.println("[FlightLog] format failed - logging disabled");
            _mounted = false;
            return;
        }
        Serial.println("[FlightLog] partition formatted OK");
    }
    _mounted = true;

    // 读取上次写入状态
    File meta = LittleFS.open("/meta.bin", "r");
    if (meta) {
        meta.read((uint8_t *)&_file_index,    sizeof(_file_index));
        meta.read((uint8_t *)&_file_records,  sizeof(_file_records));
        meta.read((uint8_t *)&_total_records, sizeof(_total_records));
        meta.close();
    }

    Serial.printf("[FlightLog] ready, total=%u records (~%.1fh), file=%u\n",
                  _total_records,
                  _total_records / 360.0f,
                  _file_index);
}

void FlightLog::write(const RIDData &data)
{
    snprintf(_path, sizeof(_path), "/log_%05u.bin", (unsigned)_file_index);

    // 新文件用 "w"，追加用 "a"
    File f = LittleFS.open(_path, _file_records == 0 ? "w" : "a");
    if (!f) {
        Serial.printf("[FlightLog] open failed: %s\n", _path);
        return;
    }

    LogRecord rec{};
    rec.timestamp_ms   = data.timestamp_ms;

    // 位置：location_valid=false 时写入"未知"标记（0x7FFFFFFF 对应 NaN 经纬度）
    // 编码器用 0xFFFFFFFF 表示未知，存储层同样用 INT32_MAX 作为哨兵
    if (data.location_valid) {
        rec.lat_e7 = (int32_t)(data.lat * 1e7);
        rec.lon_e7 = (int32_t)(data.lon * 1e7);
    } else {
        rec.lat_e7 = INT32_MAX;  // 未知标记
        rec.lon_e7 = INT32_MAX;
    }

    // 高度/速度：MAVLink 未知值为 -1000m，编码为 0（与"未知"约定一致）
    rec.geo_alt_x2     = isnan(data.geo_alt_m)  ? 0 : (int16_t)((data.geo_alt_m  + 1000.0f) * 2.0f);
    rec.rel_alt_x2     = isnan(data.rel_alt_m)  ? 0 : (int16_t)((data.rel_alt_m  + 9000.0f) * 2.0f);
    rec.ground_spd_x10 = isnan(data.ground_speed_ms) ? 0 : (int16_t)(data.ground_speed_ms * 10.0f);
    rec.track_x10      = isnan(data.track_deg)  ? 0 : (int16_t)(data.track_deg * 10.0f);
    {
        float abs_v = isnan(data.vert_speed_ms) ? 0.0f : fabsf(data.vert_speed_ms);
        uint8_t enc = (uint8_t)(abs_v * 2.0f);
        if (enc > 126) enc = 126;
        rec.vert_spd = (!isnan(data.vert_speed_ms) && data.vert_speed_ms < 0)
                       ? (int8_t)(int8_t(0x80) | (int8_t)enc) : (int8_t)enc;
    }
    rec.op_status  = (uint8_t)data.op_status;
    rec.horiz_acc  = (uint8_t)data.horiz_acc;
    rec.vert_acc   = (uint8_t)data.vert_acc;

    // 检查写入结果，只有成功才推进计数器
    const size_t written = f.write((uint8_t *)&rec, sizeof(rec));
    f.close();

    if (written != sizeof(rec)) {
        Serial.printf("[FlightLog] write failed: %s\n", _path);
        return;
    }

    _file_records++;
    _total_records++;

    // 当前文件写满，切换到下一个（循环覆盖旧文件）
    if (_file_records >= RECORDS_PER_FILE) {
        _file_index   = (_file_index + 1) % MAX_FILES;
        _file_records = 0;
    }

    // 每 10 条持久化一次元数据（约 100s），减少断电丢失窗口
    if (_total_records % 10 == 0) {
        save_meta();
    }
}

void FlightLog::save_meta()
{
    File meta = LittleFS.open("/meta.bin", "w");
    if (meta) {
        meta.write((uint8_t *)&_file_index,    sizeof(_file_index));
        meta.write((uint8_t *)&_file_records,  sizeof(_file_records));
        meta.write((uint8_t *)&_total_records, sizeof(_total_records));
        meta.close();
    }
}

uint32_t FlightLog::record_count()
{
    return _total_records;
}
