/*
 * Wi-Fi 广播发送实现
 *
 * 使用 esp_wifi_80211_tx 发送原始 802.11 帧，
 * 将国标 GB 46750-2025 数据包嵌入 Beacon 帧 Vendor IE。
 *
 * 正常飞行模式：AP 热点关闭，Wi-Fi 仅用于 RID 广播（§6.1.2）
 * 配置模式：Wi-Fi 切换为 AP 热点，由 WebServer 管理
 */

#include "wifi_tx.h"
#include "../system/parameters.h"
#include "../gb46750/encoder.h"

#include <esp_wifi.h>
#include <WiFi.h>

// Vendor IE OUI（自定义，用于国标 RID 标识）
// 使用 ASTM OUI FA:0B:BC 保持与现有接收端兼容
#define RID_OUI_0  0xFA
#define RID_OUI_1  0x0B
#define RID_OUI_2  0xBC
#define RID_OUI_TYPE 0x0D

bool WiFi_TX::init()
{
    if (_initialised) return true;
    _initialised = true;

    // 随机本地 MAC，避免追踪
    for (int i = 0; i < 6; i++) _mac[i] = (uint8_t)(esp_random() & 0xFF);
    _mac[0] |= 0x02;   // 本地管理位
    _mac[0] &= 0xFE;   // 单播位

    esp_base_mac_addr_set(_mac);

    // 启动 SoftAP（隐藏，不允许连接）用于发送原始帧
    // 使用随机 SSID 避免空 SSID 在不同 Arduino-ESP32 版本上行为不一致
    char hidden_ssid[16];
    snprintf(hidden_ssid, sizeof(hidden_ssid), "RID-%02X%02X%02X",
             _mac[3], _mac[4], _mac[5]);
    WiFi.softAP(hidden_ssid, nullptr,
                Parameters::get_uint8(PARAM_WIFI_CH), /*hidden=*/true, /*max_conn=*/0);

    if (esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20) != ESP_OK) {
        return false;
    }

    // 功率：满足 §6.1.4 轻型及以上 2.4GHz ≥ 11 dBm
    // esp_wifi_set_max_tx_power 单位为 0.25 dBm
    esp_wifi_set_max_tx_power(52);  // 52 * 0.25 = 13 dBm

    return true;
}

bool WiFi_TX::transmit(const RIDData &data)
{
    init();

    // 编码国标数据包
    uint8_t gb_buf[GB_MAX_PACKET_LEN];
    int gb_len = GB46750Encoder::encode(data, gb_buf, sizeof(gb_buf));
    if (gb_len <= 0) return false;

    // vendor_ie_data_t.payload 是零长数组，不能直接在栈上使用。
    // 按实际大小分配：element_id(1) + length(1) + oui(3) + oui_type(1) + payload(gb_len)
    const size_t ie_size = sizeof(vendor_ie_data_t) + gb_len;
    vendor_ie_data_t *ie = static_cast<vendor_ie_data_t *>(malloc(ie_size));
    if (!ie) return false;

    ie->element_id      = WIFI_VENDOR_IE_ELEMENT_ID;  // 0xDD
    ie->vendor_oui[0]   = RID_OUI_0;
    ie->vendor_oui[1]   = RID_OUI_1;
    ie->vendor_oui[2]   = RID_OUI_2;
    ie->vendor_oui_type = RID_OUI_TYPE;
    ie->length          = (uint8_t)(4 + gb_len);  // oui(3) + type(1) + payload
    memcpy(ie->payload, gb_buf, gb_len);

    // 先移除旧 IE，再设置新 IE（Beacon + Probe Response）
    esp_wifi_set_vendor_ie(false, WIFI_VND_IE_TYPE_BEACON,     WIFI_VND_IE_ID_0, ie);
    bool ok = (esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, ie) == ESP_OK);
    if (ok) {
        esp_wifi_set_vendor_ie(false, WIFI_VND_IE_TYPE_PROBE_RESP, WIFI_VND_IE_ID_0, ie);
        ok = (esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_PROBE_RESP, WIFI_VND_IE_ID_0, ie) == ESP_OK);
    }

    free(ie);

    if (!ok) Serial.println("[WiFi] set_vendor_ie failed");
    return ok;
}
