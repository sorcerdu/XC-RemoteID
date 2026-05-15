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

void FlightLog::init()
{
    // 挂载 fllog 分区（LittleFS）
    if (!LittleFS.begin(true, "/fllog", 10, "fllog")) {
        Serial.println("[FlightLog] mount failed");
        return;
    }

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
    if (!data.location_valid) return;

    snprintf(_path, sizeof(_path), "/log_%05u.bin", (unsigned)_file_index);

    // 新文件用 "w"，追加用 "a"
    File f = LittleFS.open(_path, _file_records == 0 ? "w" : "a");
    if (!f) {
        Serial.printf("[FlightLog] open failed: %s\n", _path);
        return;
    }

    LogRecord rec{};
    rec.timestamp_ms   = data.timestamp_ms;
    rec.lat_e7         = (int32_t)(data.lat * 1e7);
    rec.lon_e7         = (int32_t)(data.lon * 1e7);
    rec.geo_alt_x2     = (int16_t)((data.geo_alt_m + 1000.0f) * 2.0f);
    rec.rel_alt_x2     = (int16_t)((data.rel_alt_m + 9000.0f) * 2.0f);
    rec.ground_spd_x10 = (int16_t)(data.ground_speed_ms * 10.0f);
    rec.track_x10      = (int16_t)(data.track_deg * 10.0f);
    {
        float abs_v = fabsf(data.vert_speed_ms);
        uint8_t enc = (uint8_t)(abs_v * 2.0f);
        if (enc > 127) enc = 127;
        rec.vert_spd = (data.vert_speed_ms < 0)
                       ? (int8_t)(0x80 | enc) : (int8_t)enc;
    }
    rec.op_status  = (uint8_t)data.op_status;
    rec.horiz_acc  = (uint8_t)data.horiz_acc;
    rec.vert_acc   = (uint8_t)data.vert_acc;

    f.write((uint8_t *)&rec, sizeof(rec));
    f.close();

    _file_records++;
    _total_records++;

    // 当前文件写满，切换到下一个（循环覆盖旧文件）
    if (_file_records >= RECORDS_PER_FILE) {
        _file_index   = (_file_index + 1) % MAX_FILES;
        _file_records = 0;
    }

    // 每 100 条持久化一次元数据
    if (_total_records % 100 == 0) {
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
