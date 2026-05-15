/*
 * Web 配置服务器实现
 */

#include "web_server.h"
#include "../system/parameters.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Arduino.h>

const char *XCWebServer::AP_SSID = nullptr;
const char *XCWebServer::AP_PASS = "12345678";

static ::WebServer s_server(80);
static char s_ap_ssid[24];
static const IPAddress AP_IP(10, 0, 0, 1);
static const IPAddress AP_GATEWAY(10, 0, 0, 1);
static const IPAddress AP_SUBNET(255, 255, 255, 0);

static const char PAGE_HTML[] PROGMEM = R"====(<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#1a56db">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<title>XC-RemoteID 配置</title>
<style>
  :root {
    --bg:      #eef1f6;
    --card:    #ffffff;
    --accent:  #1a56db;
    --accent2: #0e9f6e;
    --text:    #111827;
    --muted:   #6b7280;
    --danger:  #e02424;
    --radius:  12px;
    --shadow:  0 2px 12px rgba(0,0,0,.07);
  }

  * { box-sizing: border-box; margin: 0; padding: 0; }

  body {
    background: var(--bg);
    color: var(--text);
    font-family: -apple-system, 'Helvetica Neue', Arial, sans-serif;
    font-size: 14px;
    padding-bottom: 48px;
  }

  /* Header */
  .header {
    background: var(--accent);
    padding: 14px 20px;
    display: flex; align-items: center; gap: 12px;
  }
  .header-icon {
    width: 32px; height: 32px;
    background: rgba(255,255,255,.2);
    border-radius: 8px;
    display: flex; align-items: center; justify-content: center;
    font-size: 9px; font-weight: 800; color: #fff;
    letter-spacing: .04em; flex-shrink: 0;
  }
  .header-title { font-size: 15px; font-weight: 700; color: #fff; }
  .header-sub   { font-size: 11px; color: rgba(255,255,255,.6); margin-top: 1px; }
  .status-pill  {
    margin-left: auto;
    background: rgba(255,255,255,.15);
    border-radius: 20px; padding: 3px 10px;
    display: flex; align-items: center; gap: 5px;
    font-size: 10px; font-weight: 600; color: rgba(255,255,255,.9);
    letter-spacing: .06em;
  }
  .dot {
    width: 6px; height: 6px; border-radius: 50%;
    background: #6ee7b7; box-shadow: 0 0 5px #6ee7b7;
    animation: pulse 2s infinite;
  }
  @keyframes pulse { 0%,100%{opacity:1} 50%{opacity:.4} }

  /* Layout */
  .container { max-width: 480px; margin: 0 auto; padding: 20px 16px 0; }

  /* Banner */
  .banner {
    background: #fffbeb;
    border-radius: var(--radius);
    padding: 12px 14px;
    font-size: 12px; color: #78350f;
    line-height: 1.6; margin-bottom: 16px;
    box-shadow: var(--shadow);
  }
  .banner strong { font-weight: 700; }

  /* Card */
  .card {
    background: var(--card);
    border-radius: var(--radius);
    padding: 18px 16px;
    margin-bottom: 12px;
    box-shadow: var(--shadow);
  }
  .card-title {
    font-size: 11px; font-weight: 700;
    letter-spacing: .08em; text-transform: uppercase;
    color: var(--muted); margin-bottom: 16px;
  }

  /* Field */
  .field { margin-bottom: 16px; }
  .field:last-child { margin-bottom: 0; }

  .field-label {
    display: flex; align-items: center; gap: 6px;
    margin-bottom: 5px;
  }
  .field-name { font-size: 13px; font-weight: 600; color: var(--text); }
  .tag-required {
    font-size: 10px; padding: 1px 5px; border-radius: 4px;
    font-weight: 600; letter-spacing: .04em;
    background: #fef3c7; color: #92400e;
  }
  .tag-optional {
    font-size: 10px; padding: 1px 5px; border-radius: 4px;
    font-weight: 600; letter-spacing: .04em;
    background: #f3f4f6; color: var(--muted);
  }
  .field-desc {
    font-size: 12px; color: var(--muted);
    line-height: 1.55; margin-bottom: 7px;
  }
  .field-desc code {
    font-family: 'SF Mono', 'Fira Code', monospace; font-size: 11px;
    color: var(--accent2); background: #f0fdf4;
    padding: 1px 4px; border-radius: 3px;
  }
  .field-desc strong { color: var(--accent2); font-weight: 600; }
  .field-hint { font-size: 11px; color: var(--muted); margin-top: 5px; opacity: .65; }

  /* Inputs */
  input[type=text], input[type=number], select {
    width: 100%;
    background: #f9fafb;
    border: none;
    border-radius: 8px;
    color: var(--text);
    font-family: inherit; font-size: 14px;
    padding: 10px 12px;
    outline: none;
    transition: box-shadow .15s, background .15s;
    -webkit-appearance: none; appearance: none;
  }
  input:focus, select:focus {
    background: #fff;
    box-shadow: 0 0 0 3px rgba(26,86,219,.15);
  }
  input::placeholder { color: #9ca3af; }
  select {
    background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='10' height='6'%3E%3Cpath d='M0 0l5 6 5-6z' fill='%236b7280'/%3E%3C/svg%3E");
    background-repeat: no-repeat;
    background-position: right 12px center;
    padding-right: 32px; cursor: pointer;
  }

  /* Pin row */
  .pin-row { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
  .pin-label { font-size: 12px; color: var(--muted); margin-bottom: 5px; font-weight: 500; }

  /* Buttons */
  .btn {
    width: 100%; padding: 12px 16px; border: none;
    border-radius: var(--radius);
    font-family: inherit; font-size: 14px; font-weight: 600;
    cursor: pointer;
    transition: opacity .15s, transform .1s;
    display: flex; align-items: center; justify-content: center;
  }
  .btn:active { transform: scale(.98); }
  .btn-primary {
    background: var(--accent); color: #fff;
    box-shadow: 0 4px 14px rgba(26,86,219,.3);
    margin-bottom: 10px;
  }
  .btn-primary:hover { opacity: .92; }
  .btn-ghost {
    background: var(--card); color: var(--danger);
    box-shadow: var(--shadow);
  }
  .btn-ghost:hover { opacity: .85; }

  .footer {
    text-align: center; font-size: 11px;
    color: var(--muted); opacity: .4;
    margin-top: 28px; letter-spacing: .05em;
  }
</style>
</head>
<body>

<div class="header">
  <div class="header-icon">RID</div>
  <div>
    <div class="header-title">XC-RemoteID</div>
    <div class="header-sub">GB 46750-2025 配置</div>
  </div>
  <div class="status-pill"><div class="dot"></div>CONFIG MODE</div>
</div>

<div class="container">

  <div class="banner">
    <strong>首次配置</strong> — 填写无人机身份信息后保存，设备将重启并进入正常广播模式。<br>
    所有字段均存储在设备 NVS Flash 中，断电不丢失。
  </div>

  <form method="POST" action="/save">

    <div class="card">
      <div class="card-title">无人机身份</div>

      <div class="field">
        <div class="field-label">
          <span class="field-name">唯一产品识别码</span>
          <span class="tag-required">必填</span>
        </div>
        <div class="field-desc">机身出厂序列号，符合 GB/T 41300，最多 20 位。</div>
        <input type="text" name="uas_id" maxlength="20" placeholder="例：1ZNBJ1C0D25678" value="">
        <div class="field-hint">印在机身标签或包装盒上</div>
      </div>

      <div class="field">
        <div class="field-label">
          <span class="field-name">实名登记标志</span>
          <span class="tag-required">必填</span>
        </div>
        <div class="field-desc">
          民航局实名登记号码的<strong>最后 8 位</strong>。
          如登记号 <code>CAAC-2024-BJ-00123456</code> → 填 <code>00123456</code>
        </div>
        <input type="text" name="reg_mark" maxlength="8" placeholder="例：00123456" value="">
        <div class="field-hint">登记平台：uas.caac.gov.cn</div>
      </div>
    </div>

    <div class="card">
      <div class="card-title">无人机分类</div>

      <div class="field">
        <div class="field-label">
          <span class="field-name">运行类别</span>
          <span class="tag-required">必填</span>
        </div>
        <select name="op_cat">
          <option value="0">未定义</option>
          <option value="1" selected>开放类（消费级，无需审批）</option>
          <option value="2">特定类（需运营许可）</option>
          <option value="3">审定类</option>
        </select>
      </div>

      <div class="field">
        <div class="field-label">
          <span class="field-name">无人机分类</span>
          <span class="tag-required">必填</span>
        </div>
        <select name="ua_class">
          <option value="0">微型（&lt;0.25 kg）</option>
          <option value="1" selected>轻型（≤4 kg）</option>
          <option value="2">小型（≤15 kg）</option>
          <option value="3">中型（≤150 kg）</option>
          <option value="4">大型（&gt;150 kg）</option>
        </select>
      </div>
    </div>

    <div class="card">
      <div class="card-title">飞控串口（MAVLink）</div>

      <div class="field">
        <div class="pin-row">
          <div>
            <div class="pin-label">RX 引脚（接飞控 TX）</div>
            <input type="number" name="uart_rx" min="0" max="48" value="16">
          </div>
          <div>
            <div class="pin-label">TX 引脚（接飞控 RX）</div>
            <input type="number" name="uart_tx" min="0" max="48" value="17">
          </div>
        </div>
      </div>

      <div class="field">
        <div class="field-label">
          <span class="field-name">波特率</span>
          <span class="tag-optional">可选</span>
        </div>
        <select name="baud">
          <option value="9600">9600</option>
          <option value="19200">19200</option>
          <option value="38400">38400</option>
          <option value="57600" selected>57600（ArduPilot 默认）</option>
          <option value="115200">115200（iNav 默认）</option>
          <option value="230400">230400</option>
          <option value="460800">460800</option>
          <option value="921600">921600</option>
        </select>
      </div>
    </div>

    <button class="btn btn-primary" type="submit">保存并重启</button>

  </form>

  <div class="footer">XC-RemoteID · GB 46750-2025 · CONFIG MODE</div>

</div>

<script>
fetch('/status').then(r=>r.json()).then(d=>{
  if(d.uas_id)   document.querySelector('[name=uas_id]').value=d.uas_id;
  if(d.reg_mark) document.querySelector('[name=reg_mark]').value=d.reg_mark;
  if(d.uart_rx!==undefined) document.querySelector('[name=uart_rx]').value=d.uart_rx;
  if(d.uart_tx!==undefined) document.querySelector('[name=uart_tx]').value=d.uart_tx;
  if(d.baudrate) document.querySelector('[name=baud]').value=d.baudrate;
  if(d.op_cat!==undefined)  document.querySelector('[name=op_cat]').value=d.op_cat;
  if(d.ua_class!==undefined) document.querySelector('[name=ua_class]').value=d.ua_class;
}).catch(()=>{});
</script>
</body>
</html>
)====";

void XCWebServer::start_ap()
{
    if (_started) return;
    _started = true;

    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(s_ap_ssid, sizeof(s_ap_ssid), "XC-RID-%02X%02X%02X", mac[3], mac[4], mac[5]);

    WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
    WiFi.softAP(s_ap_ssid, AP_PASS);
    Serial.printf("[WebServer] AP: %s  IP: %s\n",
                  s_ap_ssid, WiFi.softAPIP().toString().c_str());

    s_server.on("/",       [this]{ handle_root();   });
    s_server.on("/save",   HTTP_POST, [this]{ handle_save();   });
    s_server.on("/status", [this]{ handle_status(); });
    s_server.on("/reboot", HTTP_POST, [this]{ handle_reboot(); });
    s_server.begin();
}

void XCWebServer::update()
{
    s_server.handleClient();
}

void XCWebServer::stop()
{
    s_server.stop();
    WiFi.softAPdisconnect(true);
    _started = false;
}

void XCWebServer::handle_root()
{
    s_server.send_P(200, "text/html; charset=utf-8", PAGE_HTML);
}

void XCWebServer::handle_save()
{
    // ── 输入校验 ──────────────────────────────────────────────────────────────
    String uas_id   = s_server.hasArg("uas_id")   ? s_server.arg("uas_id")   : "";
    String reg_mark = s_server.hasArg("reg_mark")  ? s_server.arg("reg_mark") : "";
    String err;

    uas_id.trim();
    reg_mark.trim();

    if (uas_id.length() == 0) {
        err = "唯一产品识别码不能为空";
    } else if (uas_id.length() > 20) {
        err = "唯一产品识别码最多 20 位";
    } else if (reg_mark.length() == 0) {
        err = "实名登记标志不能为空";
    } else if (reg_mark.length() > 8) {
        err = "实名登记标志最多 8 位";
    }

    uint8_t op_cat   = s_server.hasArg("op_cat")   ? (uint8_t)s_server.arg("op_cat").toInt()   : 0;
    uint8_t ua_class = s_server.hasArg("ua_class")  ? (uint8_t)s_server.arg("ua_class").toInt() : 0;
    if (op_cat > 3)   err = "运行类别值无效（0~3）";
    if (ua_class > 4) err = "无人机分类值无效（0~4）";

    int uart_rx = s_server.hasArg("uart_rx") ? s_server.arg("uart_rx").toInt() : -1;
    int uart_tx = s_server.hasArg("uart_tx") ? s_server.arg("uart_tx").toInt() : -1;
    if (uart_rx < 0 || uart_rx > 48) err = "RX 引脚超出范围（0~48）";
    if (uart_tx < 0 || uart_tx > 48) err = "TX 引脚超出范围（0~48）";
    if (uart_rx == uart_tx)          err = "RX 和 TX 引脚不能相同";

    uint32_t baud = s_server.hasArg("baud") ? (uint32_t)s_server.arg("baud").toInt() : 0;
    if (baud == 0) err = "波特率不能为 0";

    if (err.length() > 0) {
        s_server.send(400, "text/html; charset=utf-8",
            String("<html><head><meta charset='UTF-8'></head><body>"
                   "<p style='font-family:sans-serif;padding:20px;color:#e02424'>配置错误：") +
            err + " — <a href='/'>返回</a></p></body></html>");
        return;
    }

    // ── 保存 ──────────────────────────────────────────────────────────────────
    Parameters::set_str(PARAM_UAS_ID,      uas_id.c_str());
    Parameters::set_str(PARAM_REG_MARK,    reg_mark.c_str());
    Parameters::set_uint8(PARAM_OP_CATEGORY, op_cat);
    Parameters::set_uint8(PARAM_UA_CLASS,    ua_class);
    Parameters::set_uint32(PARAM_UART_RX,    (uint32_t)uart_rx);
    Parameters::set_uint32(PARAM_UART_TX,    (uint32_t)uart_tx);
    Parameters::set_uint32(PARAM_BAUDRATE,   baud);
    Parameters::set_uint8(PARAM_CONFIGURED, 1);

    s_server.send(200, "text/html; charset=utf-8",
        "<html><head><meta charset='UTF-8'>"
        "<meta name='theme-color' content='#1a56db'></head><body>"
        "<p style='font-family:sans-serif;padding:20px;color:#111827'>已保存，正在重启...</p>"
        "</body></html>");
    delay(300);
    ESP.restart();
}

void XCWebServer::handle_status()
{
    String json = "{";
    json += "\"uas_id\":\"";    json += Parameters::get_str(PARAM_UAS_ID);    json += "\",";
    json += "\"reg_mark\":\"";  json += Parameters::get_str(PARAM_REG_MARK);  json += "\",";
    json += "\"op_cat\":";       json += Parameters::get_uint8(PARAM_OP_CATEGORY); json += ",";
    json += "\"ua_class\":";     json += Parameters::get_uint8(PARAM_UA_CLASS);    json += ",";
    json += "\"uart_rx\":";      json += Parameters::get_uart_rx_pin();             json += ",";
    json += "\"uart_tx\":";      json += Parameters::get_uart_tx_pin();             json += ",";
    json += "\"baudrate\":";     json += Parameters::get_baudrate();
    json += "}";
    s_server.send(200, "application/json", json);
}

void XCWebServer::handle_reboot()
{
    s_server.send(200, "text/plain", "Rebooting...");
    delay(200);
    ESP.restart();
}
