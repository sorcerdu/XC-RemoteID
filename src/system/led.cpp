/*
 * LED 状态指示实现
 * 兼容单色 GPIO LED 和 WS2812 RGB LED
 */

#include "led.h"
#include <Arduino.h>

#ifndef PIN_LED
#define PIN_LED 8
#endif
#ifndef LED_WS2812
#define LED_WS2812 0
#endif

#if LED_WS2812
#include <Adafruit_NeoPixel.h>
static Adafruit_NeoPixel s_pixel(1, PIN_LED, NEO_GRB + NEO_KHZ800);
#endif

Led::State    Led::_state           = Led::State::INIT;
uint32_t      Led::_last_toggle_ms  = 0;
bool          Led::_led_on          = false;

void Led::init()
{
#if LED_WS2812
    s_pixel.begin();
    s_pixel.setBrightness(30);
    s_pixel.show();
#else
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);
#endif
}

void Led::set(State s)
{
    _state = s;
    _last_toggle_ms = 0;
    _led_on = false;
}

void Led::set_color(uint8_t r, uint8_t g, uint8_t b)
{
#if LED_WS2812
    s_pixel.setPixelColor(0, s_pixel.Color(r, g, b));
    s_pixel.show();
#else
    // 单色 LED 只用亮/灭
    (void)r; (void)g; (void)b;
#endif
}

void Led::set_gpio(bool on)
{
#if LED_WS2812
    // WS2812 由 set_color 控制
    (void)on;
#else
    digitalWrite(PIN_LED, on ? HIGH : LOW);
#endif
}

void Led::update()
{
    const uint32_t now = millis();

    struct Pattern {
        uint8_t  r, g, b;
        uint32_t period_ms;   // 0 = 常亮
    };

    static const Pattern patterns[] = {
        /* INIT      */ {0,   0,   255, 1000},
        /* CONFIG    */ {0,   0,   255, 200 },
        /* WAIT_DATA */ {255, 200, 0,   800 },
        /* OK        */ {0,   255, 0,   0   },
        /* RID_FAIL  */ {255, 0,   0,   200 },
        /* ERROR     */ {255, 0,   0,   0   },
    };

    const auto &p = patterns[static_cast<int>(_state)];

    if (p.period_ms == 0) {
        // 常亮
        set_color(p.r, p.g, p.b);
        set_gpio(true);
        return;
    }

    if (now - _last_toggle_ms >= p.period_ms / 2) {
        _last_toggle_ms = now;
        _led_on = !_led_on;
        if (_led_on) {
            set_color(p.r, p.g, p.b);
            set_gpio(true);
        } else {
            set_color(0, 0, 0);
            set_gpio(false);
        }
    }
}
