/*
 * 模拟飞行数据（仅用于测试广播效果）
 *
 * 启用：在 platformio.ini 对应 env 的 build_flags 加 -DMOCK_DATA
 * 剥离：删除该行 flag 即可，不影响任何其他代码
 *
 * 模拟内容：
 *   - 无人机在北京天安门广场上空绕圆飞行，高度 50m
 *   - 每次调用 MockData::update() 推进一步
 */
#pragma once

#ifdef MOCK_DATA

#include "../gb46750/encoder.h"

class MockData {
public:
    // 初始化，填入基础身份信息
    static void init(RIDData &data);

    // 每次 loop() 调用，更新位置/速度/时间戳
    static void update(RIDData &data);

private:
    static float  _angle_deg;   // 当前绕圆角度
    static uint32_t _last_ms;
};

#endif // MOCK_DATA
