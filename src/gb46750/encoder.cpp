/*
 * GB 46750-2025 数据包编码器实现
 *
 * 数据包格式（§5.2.1）：
 *   [数据类型 1B][版本号 1B][数据长度 1B][数据标识 3+NB][数据内容项...]
 *
 * 字节序：
 *   位置/高度/速度字段 → 小端序（表3注明）
 *   唯一产品识别码/实名登记标志 → 大端序（ASCII）
 */

#include "encoder.h"
#include <string.h>
#include <math.h>

// ── 小端序写入辅助 ────────────────────────────────────────────────────────────

void GB46750Encoder::write_le16(uint8_t *p, uint16_t v)
{
    p[0] = v & 0xFF;
    p[1] = (v >> 8) & 0xFF;
}

void GB46750Encoder::write_le32(uint8_t *p, uint32_t v)
{
    p[0] = v & 0xFF;
    p[1] = (v >> 8) & 0xFF;
    p[2] = (v >> 16) & 0xFF;
    p[3] = (v >> 24) & 0xFF;
}

void GB46750Encoder::write_le48(uint8_t *p, uint64_t v)
{
    p[0] = v & 0xFF;
    p[1] = (v >> 8) & 0xFF;
    p[2] = (v >> 16) & 0xFF;
    p[3] = (v >> 24) & 0xFF;
    p[4] = (v >> 32) & 0xFF;
    p[5] = (v >> 40) & 0xFF;
}

// ── 各字段编码 ────────────────────────────────────────────────────────────────

// 001 唯一产品识别码 20字节 ASCII 左对齐，不足后补 NULL（与 reg_mark 保持一致）
int GB46750Encoder::encode_uas_id(const RIDData &d, uint8_t *p)
{
    memset(p, 0, 20);
    size_t len = strnlen(d.uas_id, 20);
    memcpy(p, d.uas_id, len);
    return 20;
}

// 002 实名登记标志 8字节 ASCII 左对齐，不足后补 NULL
int GB46750Encoder::encode_reg_mark(const RIDData &d, uint8_t *p)
{
    memset(p, 0, 8);
    size_t len = strnlen(d.reg_mark, 8);
    memcpy(p, d.reg_mark, len);
    return 8;
}

// 003 运行类别 1字节
int GB46750Encoder::encode_op_category(const RIDData &d, uint8_t *p)
{
    p[0] = static_cast<uint8_t>(d.op_category);
    return 1;
}

// 004 无人机分类 1字节
int GB46750Encoder::encode_ua_class(const RIDData &d, uint8_t *p)
{
    p[0] = static_cast<uint8_t>(d.ua_class);
    return 1;
}

// 005 遥控站位置类型 1字节
int GB46750Encoder::encode_gcs_pos_type(const RIDData &d, uint8_t *p)
{
    p[0] = static_cast<uint8_t>(d.gcs_pos_type);
    return 1;
}

// 006 遥控站位置 8字节 小端序 经度|纬度 各32位，编码值=实际值×10^7
// 未知时取 0xFFFFFFFF
int GB46750Encoder::encode_gcs_pos(const RIDData &d, uint8_t *p)
{
    if (!d.gcs_pos_valid) {
        // 未知
        memset(p, 0xFF, 8);
    } else {
        int32_t lon_enc = static_cast<int32_t>(d.gcs_lon * 1e7);
        int32_t lat_enc = static_cast<int32_t>(d.gcs_lat * 1e7);
        write_le32(p,     static_cast<uint32_t>(lon_enc));
        write_le32(p + 4, static_cast<uint32_t>(lat_enc));
    }
    return 8;
}

// 007 遥控站高度 2字节 小端序 编码值=(实际值+1000)×2，分辨率0.5m，未知取0
int GB46750Encoder::encode_gcs_alt(const RIDData &d, uint8_t *p)
{
    uint16_t enc = 0;
    if (d.gcs_pos_valid && !isnan(d.gcs_alt)) {
        float v = (d.gcs_alt + 1000.0f) * 2.0f;
        if (v < 0) v = 0;
        if (v > 65534) v = 65534;
        enc = static_cast<uint16_t>(v);
    }
    write_le16(p, enc);
    return 2;
}

// 008 无人机位置 8字节 小端序 经度|纬度 各32位，编码值=实际值×10^7
int GB46750Encoder::encode_ua_pos(const RIDData &d, uint8_t *p)
{
    if (!d.location_valid) {
        memset(p, 0xFF, 8);
    } else {
        int32_t lon_enc = static_cast<int32_t>(d.lon * 1e7);
        int32_t lat_enc = static_cast<int32_t>(d.lat * 1e7);
        write_le32(p,     static_cast<uint32_t>(lon_enc));
        write_le32(p + 4, static_cast<uint32_t>(lat_enc));
    }
    return 8;
}

// 009 航迹角 2字节 小端序 编码值=实际值×10，向下取整，0~3599，未知0xFFFF
int GB46750Encoder::encode_track(const RIDData &d, uint8_t *p)
{
    uint16_t enc = 0xFFFF;
    if (d.location_valid && !isnan(d.track_deg)) {
        float deg = fmodf(d.track_deg, 360.0f);
        if (deg < 0) deg += 360.0f;
        enc = static_cast<uint16_t>(deg * 10.0f);
        if (enc > 3599) enc = 3599;
    }
    write_le16(p, enc);
    return 2;
}

// 010 地速 2字节 小端序 编码值=实际值×10，向下取整，分辨率0.1m/s，未知0xFFFF
int GB46750Encoder::encode_ground_speed(const RIDData &d, uint8_t *p)
{
    uint16_t enc = 0xFFFF;
    if (d.location_valid && !isnan(d.ground_speed_ms) && d.ground_speed_ms >= 0) {
        enc = static_cast<uint16_t>(d.ground_speed_ms * 10.0f);
    }
    write_le16(p, enc);
    return 2;
}

// 011 相对高度 2字节 小端序 编码值=(实际值+9000)×2，分辨率0.5m，未知取0
int GB46750Encoder::encode_rel_alt(const RIDData &d, uint8_t *p)
{
    uint16_t enc = 0;
    if (d.location_valid && !isnan(d.rel_alt_m)) {
        float v = (d.rel_alt_m + 9000.0f) * 2.0f;
        if (v < 0) v = 0;
        if (v > 65534) v = 65534;
        enc = static_cast<uint16_t>(v);
    }
    write_le16(p, enc);
    return 2;
}

// 012 垂直速度 1字节 第1位标志位(0=上升,1=下降)，编码值=|实际值|×2，分辨率0.5m/s，未知0xFF
int GB46750Encoder::encode_vert_speed(const RIDData &d, uint8_t *p)
{
    if (!d.location_valid || isnan(d.vert_speed_ms)) {
        p[0] = 0xFF;
    } else {
        float abs_v = fabsf(d.vert_speed_ms);
        uint8_t enc = static_cast<uint8_t>(abs_v * 2.0f);
        // 限制 ≤126：下降时 0x80|127=0xFF 与"未知"标记冲突
        if (enc > 126) enc = 126;
        uint8_t flag = (d.vert_speed_ms < 0) ? 0x80 : 0x00; // 下降时第1位为1
        p[0] = flag | enc;
    }
    return 1;
}

// 013 大地高度 2字节 小端序 编码值=(实际值+1000)×2，分辨率0.5m，未知取0
int GB46750Encoder::encode_geo_alt(const RIDData &d, uint8_t *p)
{
    uint16_t enc = 0;
    if (d.location_valid && !isnan(d.geo_alt_m)) {
        float v = (d.geo_alt_m + 1000.0f) * 2.0f;
        if (v < 0) v = 0;
        if (v > 65534) v = 65534;
        enc = static_cast<uint16_t>(v);
    }
    write_le16(p, enc);
    return 2;
}

// 014 气压高度 2字节 小端序 编码值=(实际值+1000)×2，分辨率0.5m，未知取0
int GB46750Encoder::encode_baro_alt(const RIDData &d, uint8_t *p)
{
    uint16_t enc = 0;
    if (d.location_valid && !isnan(d.baro_alt_m)) {
        float v = (d.baro_alt_m + 1000.0f) * 2.0f;
        if (v < 0) v = 0;
        if (v > 65534) v = 65534;
        enc = static_cast<uint16_t>(v);
    }
    write_le16(p, enc);
    return 2;
}

// 015 运行状态 1字节
int GB46750Encoder::encode_op_status(const RIDData &d, uint8_t *p)
{
    p[0] = static_cast<uint8_t>(d.op_status);
    return 1;
}

// 016 坐标系类型 1字节
int GB46750Encoder::encode_coord_type(const RIDData &d, uint8_t *p)
{
    p[0] = static_cast<uint8_t>(d.coord_type);
    return 1;
}

// 017 水平精度 1字节
int GB46750Encoder::encode_horiz_acc(const RIDData &d, uint8_t *p)
{
    p[0] = static_cast<uint8_t>(d.horiz_acc);
    return 1;
}

// 018 垂直精度 1字节
int GB46750Encoder::encode_vert_acc(const RIDData &d, uint8_t *p)
{
    p[0] = static_cast<uint8_t>(d.vert_acc);
    return 1;
}

// 019 速度精度 1字节
int GB46750Encoder::encode_spd_acc(const RIDData &d, uint8_t *p)
{
    p[0] = static_cast<uint8_t>(d.spd_acc);
    return 1;
}

// 020 时间戳 6字节 小端序 Unix 时间 ms，未知取0
int GB46750Encoder::encode_timestamp(const RIDData &d, uint8_t *p)
{
    write_le48(p, d.timestamp_ms);
    return 6;
}

// 021 时间戳精度 1字节
int GB46750Encoder::encode_ts_acc(const RIDData &d, uint8_t *p)
{
    p[0] = static_cast<uint8_t>(d.ts_acc);
    return 1;
}

// ── 主编码函数 ────────────────────────────────────────────────────────────────

int GB46750Encoder::encode(const RIDData &data, uint8_t *buf, size_t buf_len,
                           uint8_t version_minor)
{
    if (buf_len < GB_MAX_PACKET_LEN) {
        return -1;
    }

    // 数据内容区（先写到临时缓冲，再填长度）
    // GB_MAX_PACKET_LEN=220，减去 6 字节头（type+ver+len+bitmap×3），内容区最大 214 字节
    // 实测 21 个字段最大约 72 字节，此处留足余量
    static_assert(GB_MAX_PACKET_LEN > 6, "GB_MAX_PACKET_LEN too small");
    uint8_t content[GB_MAX_PACKET_LEN - 6];
    uint8_t bitmap[3] = {0, 0, 0};
    int content_len = 0;

    // 宏：写字段并设置标志位
#define WRITE_FIELD(byte_idx, flag, fn) \
    do { \
        int n = fn(data, content + content_len); \
        if (n > 0) { bitmap[byte_idx] |= (flag); content_len += n; } \
    } while(0)

    // 第1字节字段（必选）
    WRITE_FIELD(0, GB_FLAG_BYTE1_UAS_ID,       encode_uas_id);
    WRITE_FIELD(0, GB_FLAG_BYTE1_REG_MARK,     encode_reg_mark);
    WRITE_FIELD(0, GB_FLAG_BYTE1_OP_CATEGORY,  encode_op_category);  // 可选，始终发送
    WRITE_FIELD(0, GB_FLAG_BYTE1_UA_CLASS,     encode_ua_class);
    WRITE_FIELD(0, GB_FLAG_BYTE1_GCS_POS_TYPE, encode_gcs_pos_type);
    WRITE_FIELD(0, GB_FLAG_BYTE1_GCS_POS,      encode_gcs_pos);
    WRITE_FIELD(0, GB_FLAG_BYTE1_GCS_ALT,      encode_gcs_alt);
    bitmap[0] |= GB_FLAG_BYTE1_EXT;  // 有第2字节

    // 第2字节字段
    WRITE_FIELD(1, GB_FLAG_BYTE2_UA_POS,       encode_ua_pos);
    WRITE_FIELD(1, GB_FLAG_BYTE2_TRACK,        encode_track);
    WRITE_FIELD(1, GB_FLAG_BYTE2_GROUND_SPEED, encode_ground_speed);
    WRITE_FIELD(1, GB_FLAG_BYTE2_REL_ALT,      encode_rel_alt);      // 可选
    WRITE_FIELD(1, GB_FLAG_BYTE2_VERT_SPEED,   encode_vert_speed);   // 可选
    WRITE_FIELD(1, GB_FLAG_BYTE2_GEO_ALT,      encode_geo_alt);
    WRITE_FIELD(1, GB_FLAG_BYTE2_BARO_ALT,     encode_baro_alt);     // 可选
    bitmap[1] |= GB_FLAG_BYTE2_EXT;  // 有第3字节

    // 第3字节字段
    WRITE_FIELD(2, GB_FLAG_BYTE3_OP_STATUS,  encode_op_status);
    WRITE_FIELD(2, GB_FLAG_BYTE3_COORD_TYPE, encode_coord_type);
    WRITE_FIELD(2, GB_FLAG_BYTE3_H_ACC,      encode_horiz_acc);
    WRITE_FIELD(2, GB_FLAG_BYTE3_V_ACC,      encode_vert_acc);
    WRITE_FIELD(2, GB_FLAG_BYTE3_SPD_ACC,    encode_spd_acc);
    WRITE_FIELD(2, GB_FLAG_BYTE3_TIMESTAMP,  encode_timestamp);
    WRITE_FIELD(2, GB_FLAG_BYTE3_TS_ACC,     encode_ts_acc);
    // 第3字节扩展位不置位（无第4字节）

#undef WRITE_FIELD

    // 组装完整数据包
    int offset = 0;

    // 数据类型
    buf[offset++] = GB_DATA_TYPE;

    // 版本号：高3位固定"001"(bit7~5 = 0x20)，低5位为 version_minor
    buf[offset++] = GB_VERSION_BASE | (version_minor & 0x1F);

    // 数据长度（数据内容项字节数，不含标识字节）
    buf[offset++] = static_cast<uint8_t>(content_len);

    // 数据标识（3字节）
    buf[offset++] = bitmap[0];
    buf[offset++] = bitmap[1];
    buf[offset++] = bitmap[2];

    // 数据内容项
    memcpy(buf + offset, content, content_len);
    offset += content_len;

    return offset;
}
