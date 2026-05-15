/*
 * XC-RemoteID - GB 46750-2025 compliant Remote ID firmware
 * Main entry point
 */

#include <Arduino.h>
#include "system/parameters.h"
#include "system/led.h"
#include "system/interlock.h"
#include "transport/mavlink_input.h"
#include "broadcast/ble_tx.h"
#include "broadcast/wifi_tx.h"
#include "storage/flight_log.h"
#include "webserver/web_server.h"

#ifdef MOCK_DATA
#include "mock/mock_data.h"
static RIDData mock_rid_data{};
#endif

static MAVLinkInput mavlink;
static BLE_TX       ble;
static WiFi_TX      wifi_tx;
static XCWebServer  webserver;

enum class BootMode { NORMAL, CONFIG };
static BootMode boot_mode = BootMode::NORMAL;

static bool is_config_mode_requested()
{
    // BOOT 键（GPIO9）上电时按住进入配置模式
    pinMode(9, INPUT_PULLUP);
    delay(50);
    return (digitalRead(9) == LOW);
}

void setup()
{
    Serial.begin(115200);

    Parameters::init();
    Led::init();
    Led::set(Led::State::INIT);

    // ── 判断启动模式 ──────────────────────────────────────────────────────────
    // CONFIG 模式：首次上电（未配置）或上电时按住 BOOT 键
    // NORMAL 模式：已配置且未按 BOOT 键
#ifdef MOCK_DATA
    // MOCK 模式跳过配置检查，强制进入 NORMAL
    Serial.println("[XC-RID] MOCK mode, skip config check");
#else
    if (is_config_mode_requested() || !Parameters::is_configured()) {
        boot_mode = BootMode::CONFIG;
        Serial.println("[XC-RID] CONFIG mode - AP: XC-RID-xxxx / 12345678");
        Led::set(Led::State::CONFIG);
        webserver.start_ap();
        return;
    }
#endif

    // ── NORMAL 模式 ───────────────────────────────────────────────────────────
    Serial.println("[XC-RID] NORMAL mode");

    FlightLog::init();

#ifdef MOCK_DATA
    MockData::init(mock_rid_data);
    Serial.println("[XC-RID] Mock data: Qingpu Dianshan Lake");
#else
    mavlink.init(
        Parameters::get_uart_rx_pin(),
        Parameters::get_uart_tx_pin(),
        Parameters::get_baudrate()
    );
    Interlock::init();
    Interlock::set_mavlink(&mavlink);
#endif

    // BLE + WiFi 广播同时启动（GB 46750 §5.1.1）
    ble.init();
    wifi_tx.init();

    Led::set(Led::State::WAIT_DATA);
}

void loop()
{
    if (boot_mode == BootMode::CONFIG) {
        webserver.update();
        Led::update();
        return;
    }

    const uint32_t now_ms = millis();

#ifdef MOCK_DATA
    MockData::update(mock_rid_data);
    const RIDData &data = mock_rid_data;
    Led::set(Led::State::OK);
#else
    mavlink.update();
    const RIDData &data = mavlink.get_data();
    Interlock::update(data);
#endif

    Led::update();

    // 广播 1Hz（GB 46750 §5.1.3）
    static uint32_t last_broadcast_ms = 0;
    if (now_ms - last_broadcast_ms >= 1000) {
        last_broadcast_ms = now_ms;
        if (data.location_valid) {
            ble.transmit(data);
            wifi_tx.transmit(data);
        }
    }

    // 存储 10s（GB 46750 §5.1.8）
    static uint32_t last_storage_ms = 0;
    if (data.location_valid && now_ms - last_storage_ms >= 10000) {
        last_storage_ms = now_ms;
        FlightLog::write(data);
    }

    delay(1);
}
