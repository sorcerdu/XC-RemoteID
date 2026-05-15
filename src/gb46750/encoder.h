/*
 * GB 46750-2025 数据包编码器
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include "fields.h"

// 编码后最大包长：4字节头 + 3字节标识 + 最大数据内容
#define GB_MAX_PACKET_LEN  220

// 飞行数据（由 MAVLink 输入层填充）
struct RIDData {
    // 身份（来自参数存储）
    char    uas_id[21];         // 001 唯一产品识别码
    char    reg_mark[9];        // 002 实名登记标志（后8位）

    // 可选配置
    GBOpCategory op_category;   // 003
    GBUAClass    ua_class;      // 004

    // 遥控站
    GBGCSPosType gcs_pos_type;  // 005
    double  gcs_lat;            // 006 度
    double  gcs_lon;            // 006 度
    float   gcs_alt;            // 007 大地高度 m

    // 无人机位置
    double  lat;                // 008 度
    double  lon;                // 008 度
    float   track_deg;          // 009 航迹角 度（真北顺时针）
    float   ground_speed_ms;    // 010 地速 m/s
    float   rel_alt_m;          // 011 相对高度 m（可选）
    float   vert_speed_ms;      // 012 垂直速度 m/s（可选，正=上升）
    float   geo_alt_m;          // 013 大地高度 m
    float   baro_alt_m;         // 014 气压高度 m（可选）

    // 状态与精度
    GBOpStatus   op_status;     // 015
    GBCoordType  coord_type;    // 016
    GBHorizAcc   horiz_acc;     // 017
    GBVertAcc    vert_acc;      // 018
    GBSpdAcc     spd_acc;       // 019
    uint64_t     timestamp_ms;  // 020 Unix 时间 ms
    GBTsAcc      ts_acc;        // 021

    // 有效性标志
    bool location_valid;
    bool gcs_pos_valid;
};

class GB46750Encoder {
public:
    /*
     * 编码一个完整数据包到 buf，返回实际字节数，失败返回 -1
     * version_minor: 版本号第4~8位，0~63
     */
    static int encode(const RIDData &data, uint8_t *buf, size_t buf_len,
                      uint8_t version_minor = 0);

private:
    // 写小端序 uint16
    static void write_le16(uint8_t *p, uint16_t v);
    // 写小端序 uint32
    static void write_le32(uint8_t *p, uint32_t v);
    // 写小端序 uint64（6字节截断，用于时间戳）
    static void write_le48(uint8_t *p, uint64_t v);

    // 各字段编码
    static int encode_uas_id(const RIDData &d, uint8_t *p);
    static int encode_reg_mark(const RIDData &d, uint8_t *p);
    static int encode_op_category(const RIDData &d, uint8_t *p);
    static int encode_ua_class(const RIDData &d, uint8_t *p);
    static int encode_gcs_pos_type(const RIDData &d, uint8_t *p);
    static int encode_gcs_pos(const RIDData &d, uint8_t *p);
    static int encode_gcs_alt(const RIDData &d, uint8_t *p);
    static int encode_ua_pos(const RIDData &d, uint8_t *p);
    static int encode_track(const RIDData &d, uint8_t *p);
    static int encode_ground_speed(const RIDData &d, uint8_t *p);
    static int encode_rel_alt(const RIDData &d, uint8_t *p);
    static int encode_vert_speed(const RIDData &d, uint8_t *p);
    static int encode_geo_alt(const RIDData &d, uint8_t *p);
    static int encode_baro_alt(const RIDData &d, uint8_t *p);
    static int encode_op_status(const RIDData &d, uint8_t *p);
    static int encode_coord_type(const RIDData &d, uint8_t *p);
    static int encode_horiz_acc(const RIDData &d, uint8_t *p);
    static int encode_vert_acc(const RIDData &d, uint8_t *p);
    static int encode_spd_acc(const RIDData &d, uint8_t *p);
    static int encode_timestamp(const RIDData &d, uint8_t *p);
    static int encode_ts_acc(const RIDData &d, uint8_t *p);
};
