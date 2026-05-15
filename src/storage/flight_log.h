/*
 * 飞行数据滚动存储（GB 46750-2025 §5.1.8）
 * 要求：更新间隔 ≤10s，存储容量 ≥120 飞行小时，不可手动删除
 *
 * 实现：LittleFS 循环文件写入（fllog 分区，2.375MB，可存 172h+）
 */
#pragma once

#include "../gb46750/encoder.h"
#include <stdint.h>

class FlightLog {
public:
    static void     init();
    static void     write(const RIDData &data);
    static uint32_t record_count();

private:
    static uint32_t _total_records;
    static uint32_t _file_index;
    static uint32_t _file_records;
    static char     _path[32];

    // 每文件约 256KB（6553 条 × 40 字节）
    static const uint32_t RECORDS_PER_FILE = 6553;
    // 最多 27 个文件，总容量 172h+
    static const uint32_t MAX_FILES        = 27;

    // 紧凑存储结构（40 字节/条）
    struct LogRecord {
        uint64_t timestamp_ms;    // 8
        int32_t  lat_e7;          // 4
        int32_t  lon_e7;          // 4
        int16_t  geo_alt_x2;      // 2
        int16_t  rel_alt_x2;      // 2
        int16_t  ground_spd_x10;  // 2
        int16_t  track_x10;       // 2
        int8_t   vert_spd;        // 1
        uint8_t  op_status;       // 1
        uint8_t  horiz_acc;       // 1
        uint8_t  vert_acc;        // 1
        uint8_t  reserved[12];    // 12 → 总计 40 字节
    } __attribute__((packed));

    static void save_meta();
};
