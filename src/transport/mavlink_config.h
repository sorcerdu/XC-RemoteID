/*
 * MAVLink 配置头文件
 * 按照 ArduRemoteID 的模式正确设置 MAVLink 宏和 include 顺序
 * 包含此文件的 .cpp 必须在之后 include <generated/mavlink_helpers.h>
 * 并定义 mavlink_system 和 send buffer 函数（或 extern 引用）
 */
#pragma once

#define MAVLINK_SEPARATE_HELPERS
#define MAVLINK_NO_CONVERSION_HELPERS
#define MAVLINK_COMM_NUM_BUFFERS 1
#define MAVLINK_MAX_PAYLOAD_LEN  255
#define MAVLINK_SEND_UART_BYTES(chan, buf, len) mavlink_serial_send(chan, buf, len)

#include <mavlink2.h>

// Forward declarations（在 mavlink_helpers.h 之前必须可见）
extern mavlink_system_t mavlink_system;
void mavlink_serial_send(mavlink_channel_t chan, const uint8_t *buf, uint8_t len);

#define MAVLINK_USE_CONVENIENCE_FUNCTIONS
#include <generated/ardupilotmega/mavlink.h>
