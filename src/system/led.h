/*
 * LED 状态指示
 * 支持单色 GPIO LED 和 WS2812 RGB LED
 */
#pragma once
#include <stdint.h>

class Led {
public:
    enum class State {
        INIT,        // 初始化中：蓝色慢闪
        CONFIG,      // 配置模式：蓝色快闪
        WAIT_DATA,   // 等待飞控数据：黄色慢闪
        OK,          // 正常广播：绿色常亮
        RID_FAIL,    // RID 失效：红色快闪
        ERROR,       // 系统错误：红色常亮
    };

    static void init();
    static void set(State s);
    static void update();   // 在 loop() 中调用，处理闪烁

private:
    static State   _state;
    static uint32_t _last_toggle_ms;
    static bool    _led_on;

    static void set_color(uint8_t r, uint8_t g, uint8_t b);
    static void set_gpio(bool on);
};
