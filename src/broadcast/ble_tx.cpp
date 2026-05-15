/*
 * BLE 广播发送实现
 */

#include "ble_tx.h"
#include "../gb46750/encoder.h"
#include <Arduino.h>
#include <esp_system.h>
#include <esp_gap_ble_api.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>

#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S3)
#define HAS_BT5_EXT_ADV 1
#else
#define HAS_BT5_EXT_ADV 0
#endif

#if HAS_BT5_EXT_ADV
// 使用 1M PHY（兼容性最好，嗖嗖FLY 等 App 均可识别）
// 注：Coded PHY S8 距离更远但部分 App 不支持扫描
static esp_ble_gap_ext_adv_params_t s_ext_params = {
    .type           = ESP_BLE_GAP_SET_EXT_ADV_PROP_NONCONN_NONSCANNABLE_UNDIRECTED,
    .interval_min   = 1600,
    .interval_max   = 1600,
    .channel_map    = ADV_CHNL_ALL,
    .own_addr_type  = BLE_ADDR_TYPE_RANDOM,
    .filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST,
    .tx_power       = ESP_PWR_LVL_P9,
    .primary_phy    = ESP_BLE_GAP_PHY_1M,
    .max_skip       = 0,
    .secondary_phy  = ESP_BLE_GAP_PHY_1M,
    .sid            = 0,
    .scan_req_notif = false,
};
static BLEMultiAdvertising s_advert(1);
#else
static BLEAdvertising *s_advert = nullptr;
#endif

bool BLE_TX::init()
{
    if (_initialised) return true;
    _initialised = true;

    BLEDevice::init("XC-RID");

    uint8_t mac[6];
    for (int i = 0; i < 6; i++) mac[i] = (uint8_t)(esp_random() & 0xFF);
    mac[0] |= 0xC0;

    Serial.printf("[BLE] MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

#if HAS_BT5_EXT_ADV
    s_advert.setAdvertisingParams(0, &s_ext_params);
    s_advert.setInstanceAddress(0, mac);
    s_advert.setDuration(0, 0);  // 持续广播，无超时
#else
    s_advert = BLEDevice::getAdvertising();
    s_advert->start();
    _started = true;
#endif

    return true;
}

bool BLE_TX::transmit(const RIDData &data)
{
    init();

    uint8_t gb_buf[GB_MAX_PACKET_LEN];
    int gb_len = GB46750Encoder::encode(data, gb_buf, sizeof(gb_buf));
    if (gb_len <= 0) {
        Serial.println("[BLE] encode failed");
        return false;
    }

    // AD Structure: [len][0x16][UUID_LO][UUID_HI][subtype][counter][GB包]
    const uint8_t ad_overhead = 5;
    int payload_len = 1 + ad_overhead + gb_len;

    if ((size_t)payload_len > sizeof(_payload)) return false;

    _payload[0] = (uint8_t)(ad_overhead + gb_len);  // AD length
    _payload[1] = 0x16;   // Service Data
    _payload[2] = 0xFA;   // UUID 0xFFFA low
    _payload[3] = 0xFF;   // UUID 0xFFFA high
    _payload[4] = 0x0D;   // subtype
    _payload[5] = _counter++;
    memcpy(_payload + 6, gb_buf, gb_len);

#if HAS_BT5_EXT_ADV
    s_advert.setAdvertisingData(0, payload_len, _payload);
    if (!_started) {
        // start() 必须在 setAdvertisingData 之后调用
        s_advert.start(1, 0);  // numAdv=1, firstAdv=0
        _started = true;
        Serial.printf("[BLE] BT5 ext adv started, payload=%d bytes\n", payload_len);
    }
#else
    esp_ble_gap_config_adv_data_raw(_payload, (uint8_t)payload_len);
#endif

    return true;
}
