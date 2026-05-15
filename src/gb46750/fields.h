/*
 * GB 46750-2025 数据内容项定义
 * 表2 数据类型与标识 / 表3 数据内容项及编码取值要求
 */
#pragma once
#include <stdint.h>

// 数据包固定字段
#define GB_DATA_TYPE        0xFF   // 数据类型固定值 255
#define GB_VERSION_BASE     0x20   // 第1~3位固定"001" = 0b001xxxxx

// 数据标识位掩码（第1字节）
#define GB_FLAG_BYTE1_UAS_ID        0x80  // 001 唯一产品识别码       M
#define GB_FLAG_BYTE1_REG_MARK      0x40  // 002 实名登记标志         M
#define GB_FLAG_BYTE1_OP_CATEGORY   0x20  // 003 运行类别             O
#define GB_FLAG_BYTE1_UA_CLASS      0x10  // 004 无人机分类           M
#define GB_FLAG_BYTE1_GCS_POS_TYPE  0x08  // 005 遥控站位置类型       M
#define GB_FLAG_BYTE1_GCS_POS       0x04  // 006 遥控站位置           M
#define GB_FLAG_BYTE1_GCS_ALT       0x02  // 007 遥控站高度           M
#define GB_FLAG_BYTE1_EXT           0x01  // 扩展标志位

// 数据标识位掩码（第2字节）
#define GB_FLAG_BYTE2_UA_POS        0x80  // 008 无人机位置           M
#define GB_FLAG_BYTE2_TRACK         0x40  // 009 航迹角               M
#define GB_FLAG_BYTE2_GROUND_SPEED  0x20  // 010 地速                 M
#define GB_FLAG_BYTE2_REL_ALT       0x10  // 011 相对高度             O
#define GB_FLAG_BYTE2_VERT_SPEED    0x08  // 012 垂直速度             O
#define GB_FLAG_BYTE2_GEO_ALT       0x04  // 013 大地高度             M
#define GB_FLAG_BYTE2_BARO_ALT      0x02  // 014 气压高度             O
#define GB_FLAG_BYTE2_EXT           0x01  // 扩展标志位

// 数据标识位掩码（第3字节）
#define GB_FLAG_BYTE3_OP_STATUS     0x80  // 015 运行状态             M
#define GB_FLAG_BYTE3_COORD_TYPE    0x40  // 016 坐标系类型           M
#define GB_FLAG_BYTE3_H_ACC         0x20  // 017 水平精度             M
#define GB_FLAG_BYTE3_V_ACC         0x10  // 018 垂直精度             M
#define GB_FLAG_BYTE3_SPD_ACC       0x08  // 019 速度精度             M
#define GB_FLAG_BYTE3_TIMESTAMP     0x04  // 020 时间戳               M
#define GB_FLAG_BYTE3_TS_ACC        0x02  // 021 时间戳精度           M
#define GB_FLAG_BYTE3_EXT           0x01  // 扩展标志位

// 运行类别 (003)
enum class GBOpCategory : uint8_t {
    UNDEFINED = 0,
    OPEN      = 1,
    SPECIFIC  = 2,
    CERTIFIED = 3,
};

// 无人机分类 (004)
enum class GBUAClass : uint8_t {
    MICRO  = 0,
    LIGHT  = 1,
    SMALL  = 2,
    MEDIUM = 3,
    LARGE  = 4,
};

// 遥控站位置类型 (005)
enum class GBGCSPosType : uint8_t {
    TAKEOFF = 0,
    GCS     = 1,
};

// 运行状态 (015)
enum class GBOpStatus : uint8_t {
    NOT_REPORTED    = 0,
    GROUND          = 1,
    AIRBORNE        = 2,
    EMERGENCY       = 3,
    RID_FAIL_NORMAL = 4,
    RID_FAIL_EMERG  = 5,
};

// 坐标系类型 (016)
enum class GBCoordType : uint8_t {
    WGS84    = 0,
    CGCS2000 = 1,
};

// 水平精度 NACp (017)
enum class GBHorizAcc : uint8_t {
    UNKNOWN_OR_GTE_18520M = 0,
    LT_18520M = 1,
    LT_7410M  = 2,
    LT_3700M  = 3,
    LT_1852M  = 4,
    LT_926M   = 5,
    LT_556M   = 6,
    LT_185M   = 7,
    LT_92M    = 8,
    LT_30M    = 9,
    LT_10M    = 10,
    LT_3M     = 11,
    LT_1M     = 12,
};

// 垂直精度 GVA (018)
enum class GBVertAcc : uint8_t {
    UNKNOWN_OR_GTE_150M = 0,
    LT_150M = 1,
    LT_45M  = 2,
    LT_25M  = 3,
    LT_10M  = 4,
    LT_3M   = 5,
    LT_1M   = 6,
};

// 速度精度 NACv (019)
enum class GBSpdAcc : uint8_t {
    UNKNOWN_OR_GTE_10MS = 0,
    LT_10MS  = 1,
    LT_3MS   = 2,
    LT_1MS   = 3,
    LT_03MS  = 4,
};

// 时间戳精度 (021)
enum class GBTsAcc : uint8_t {
    UNKNOWN_OR_GT_500MS = 0,
    LTE_500MS = 1,
    LTE_400MS = 2,
    LTE_300MS = 3,
    LTE_200MS = 4,
    LTE_100MS = 5,
    LTE_50MS  = 6,
    LTE_20MS  = 7,
    LTE_10MS  = 8,
};
