# XC-RemoteID

面向 GB 46750-2025 广播式运行识别的 ESP32 固件，已实现 BLE/Wi-Fi 广播、GB 数据包编码、MAVLink 输入、本地滚动存储和起飞联锁；网络式上报待实现。使用 PlatformIO 构建。

> 本项目参考 [ArduRemoteID](https://github.com/ArduPilot/ArduRemoteID) 开源实现，针对中国国标重新设计数据协议层。

---

## 标准背景

GB 46750—2025《民用无人驾驶航空器系统运行识别规范》于 2025 年 10 月 31 日发布，2026 年 05 月 01 日实施。

本标准规定了民用无人驾驶航空器系统运行识别的信息内容、信息格式，发送、传输、接收与处理及相关系统的功能性能要求，描述了相应的证实方法。

- 标准发布页：[GB 46750—2025](https://www.caac.gov.cn/XXGK/XXGK/BZGF/BZGF_GJBZ/202601/t20260120_229783.html)
- 网络式运行识别公告：[民航局 2026-03-06 公告](https://www.caac.gov.cn/XXGK/XXGK/TZTG/202603/t20260306_230233.html)

---

## 与 ArduRemoteID（OpenDroneID）的核心差异

| 维度 | OpenDroneID (ASTM F3411) | GB 46750—2025 |
|------|--------------------------|----------------|
| 协议标准 | ASTM F3411-22a（国际） | 中国国家强制标准 |
| 数据包格式 | 固定 1 字节 type | type + version + length + bitmap 扩展结构 |
| 身份标识 | UAS_ID（20 字节，多种类型） | 唯一产品识别码（GB/T 41300）+ 实名登记标志（后 8 位） |
| 坐标系 | WGS-84 only | WGS-84 或 CGCS2000 |
| 时间戳 | 自 2019-01-01 的秒数 | Unix 时间毫秒（6 字节小端序） |
| 网络上报 | 无强制要求 | 强制要求，需缓存重发 |
| 本地存储 | 无 | 强制 ≥120h 滚动存储，不可手动删除 |
| 起飞联锁 | 无 | 强制，RID 失效禁止起飞 |
| 禁止 ADS-B | 无规定 | 明确禁止使用 ADS-B 作为运行识别方式 |

---

## 国标条款符合性说明

### ✅ 已实现

| 条款 | 内容 | 实现方式 |
|------|------|----------|
| §5.1.2 | 依靠自身动力移动全过程持续发送，不可关闭 | 固件无关闭广播接口 |
| §5.1.3 | 更新和发送时间间隔 ≤1s | 1Hz 广播循环 |
| §5.1.5 | 自检结果通知操控员 | LED 状态指示（INIT / CONFIG / WAIT_DATA / OK / RID_FAIL / ERROR） |
| §5.1.7a | 起飞前 RID 失效时不能起飞 | MAVLink `OPEN_DRONE_ID_ARM_STATUS` 联锁 |
| §5.1.7b | 飞行中失效时告警并具备处置能力 | 失效时继续广播，op_status 覆盖为 RID_FAIL（EMERGENCY 状态保留），位置字段填 0xFF（未知），飞控可据此响应处置 |
| §5.1.8 | 滚动存储运行识别信息，更新间隔 ≤10s，≥120h，不可手动删除 | LittleFS 循环文件写入，fllog 分区 2.375MB，8 个文件 × 6553 条 × 10s ≈ **145h**；固件不提供日志删除接口，首次上电自动格式化分区，此后挂载失败保留原有数据，不格式化；失效时段同样入库，op_status 字段标识失效状态 |
| §5.1.10 | 不应使用 ADS-B | 固件无 ADS-B 实现 |
| §5.2 | 广播式数据包格式、数据标识、数据内容项编码 | 完整实现 21 个数据内容项编码器 |
| §6.1.2 | 至少使用蓝牙 5.0 广播模式或 Wi-Fi 广播模式之一 | BLE 5.0 Extended Advertising + Wi-Fi Beacon |
| §6.1.3 | 蓝牙广播功率 ≥4 dBm（轻型及以上） | 固件设置 +9 dBm（`ESP_PWR_LVL_P9`） |

### ⚠️ 部分实现 / 待完善

| 条款 | 内容 | 当前状态 | 原因 |
|------|------|----------|------|
| §5.1.1 | 同时具备广播式和网络式发送功能 | 部分实现 | 广播式已实现（BLE 5.0 Extended Advertising + Wi-Fi Beacon，1Hz 持续广播）；网络式未实现，待按 UOM 对接要求实现上报逻辑 |
| §5.1.6 | 防篡改/防破坏功能设计 | 未实现 | 需要硬件安全模块或 eFuse 锁定支持，当前开发板不具备，量产硬件设计时需考虑 |
| §5.1.9 | 缓存未发送成功的网络式信息，恢复后重发 | 未实现 | 依赖网络式上报实现 |
| §5.2 时间戳精度 | 时间戳精度要求 | 依赖飞控发送 `SYSTEM_TIME` 消息 | MAVLink `OPEN_DRONE_ID_LOCATION.timestamp` 是"UTC 整点后的秒数"（0~3599s），需结合飞控发送的 `SYSTEM_TIME`（Unix µs）建立时间基准后还原完整 Unix ms。若飞控未发送 `SYSTEM_TIME`，时间戳置 0（未知）。ArduPilot 默认发送 `SYSTEM_TIME`，iNav 需确认配置 |

### ❌ 未实现

| 条款 | 内容 | 原因 | 是否在缓冲期 |
|------|------|------|-------------|
| §5.2.2 / §6.2 网络式 | 网络式运行识别扩展内容、链路和上报 | UOM 对接要求已有官方公告；本项目尚未实现网络式上报，ESP32 无蜂窝模块，需外接 4G/有线/卫星通信模块 | 是，§9 过渡期内加装模块满足基本要求即可 |
| §7（接收端） | 广播式/网络式运行识别接收与处理系统 | 本项目为发送端固件，接收端不在范围内 | — |

---

## 过渡期说明

自本文件实施之日起，不具备运行识别发送功能的民用无人驾驶航空器不应实施运行。对于已销售并在使用中的民用无人驾驶航空器系统，设备生产厂家须在本文件发布之日起 **12 个月**内，通过对民用无人驾驶航空器系统加装运行识别模块满足基本要求。针对加装运行识别模块的民用无人驾驶航空器系统给予 **36 个月过渡期**，以完全满足本文件的全部要求。过渡期结束后，所有民用无人驾驶航空器应完全满足本文件要求方可实施运行。

本项目作为加装模块方案，当前重点覆盖广播式运行识别、飞行数据存储和起飞联锁。当前满足广播式运行识别的基本要求，完整合规需补充网络式上报和防篡改/防破坏设计。

---

## 硬件支持

| 板型 | 芯片 | BLE | 配置入口 | platformio env |
|------|------|-----|----------|----------------|
| ESP32-S3 DevKitC-1 | ESP32-S3 | BT5 ✅ | 需适配（板载 BOOT 常见为 GPIO0） | `esp32s3_devkit` |
| ESP32-C3 Super Mini | ESP32-C3 | BT5 ✅ | GPIO9 | `esp32c3_supermini` |
| ESP32-C3 DevKit | ESP32-C3 | BT5 ✅ | GPIO9 | `esp32c3_devkit` |
| XIAO ESP32-C3 | ESP32-C3 | BT5 ✅ | GPIO9（板载 BOOT 键） | `xiao_esp32c3` |
| ESP32 WROOM-32 | ESP32 | BT4.2 ⚠️ | 需适配（板载 BOOT 常见为 GPIO0） | `esp32_wroom32` |

> ⚠️ WROOM-32 仅支持 BT4.2，BLE 广播包受 31 字节限制，无法承载完整 GB 46750 数据包，**不推荐用于正式部署**，仅供开发调试。
>
> 配置入口当前由固件检测 GPIO9。ESP32-S3、ESP32 WROOM-32 等板型的板载 BOOT 键通常不在 GPIO9，正式部署前需把配置入口引脚改到实际按键或外接按键。

---

## 固件刷写

从项目发布页下载对应板型的固件文件，然后选择以下任意一种方式刷写。

> 首次刷写裸开发板时，推荐使用包含 bootloader、partition table 和 app 的 merged 固件。若下载的是单独 app `.bin`，通常只适合已有正确 bootloader 和分区表的设备，刷写地址为 `0x10000`。

### 方案一：在线刷写（推荐，无需安装任何工具）

使用 [esptool.spacehuhn.com](https://esptool.spacehuhn.com) 直接在浏览器中刷写，需要 **Chrome 或 Edge**（不支持 Safari 和 Firefox）。

1. 用 USB 线连接 ESP32 开发板
2. 打开 [https://esptool.spacehuhn.com](https://esptool.spacehuhn.com)
3. 点击 **Connect**，在弹出的串口选择框中选择你的设备（通常显示为 `USB Serial` 或 `CP210x`）
4. 点击 **Add File**，上传下载好的 `.bin` 文件；merged 固件地址通常填 `0x0`，单 app 固件地址填 `0x10000`
5. 点击 **Program** 开始刷写，等待进度条完成
6. 刷写完成后断开连接，重新上电

> **ESP32-C3 / S3 注意**：首次连接前需按住 BOOT 键再插 USB，进入下载模式。

### 方案二：本地刷写

#### Flash Download Tool（Windows，图形界面）

乐鑫官方工具，适合 Windows 用户，[点击下载](https://www.espressif.com/en/support/download/other-tools)。

1. 打开工具，芯片类型选择对应型号（ESP32 / ESP32-C3 / ESP32-S3）
2. 在第一行点击 `...` 选择下载的 `.bin` 文件；merged 固件地址通常填 `0x0`，单 app 固件地址填 `0x10000`
3. 选择正确的 COM 端口，波特率选 `921600`
4. 点击 **START** 开始烧录

#### esptool.py（命令行，跨平台）

```bash
# 安装
pip install esptool

# 烧录单 app 固件（将端口替换为实际端口）
esptool.py --port /dev/ttyUSB0 --baud 921600 \
  write_flash 0x10000 XC-RemoteID-esp32c3_supermini-v1.0.0.bin

# 烧录 merged 固件
esptool.py --port /dev/ttyUSB0 --baud 921600 \
  write_flash 0x0 XC-RemoteID-esp32c3_supermini-merged-v1.0.0.bin
```

各平台端口名称参考：

| 系统 | 端口格式 |
|------|---------|
| macOS | `/dev/cu.usbserial-xxxx` 或 `/dev/cu.usbmodem-xxxx` |
| Linux | `/dev/ttyUSB0` 或 `/dev/ttyACM0` |
| Windows | `COM3`、`COM4` 等 |

> **ESP32-C3 / S3**：烧录前需按住 BOOT 键再上电进入下载模式，烧录完成后按 RST 键重启。

---

## 首次配置

上电后设备自动进入配置模式，连接 Wi-Fi 热点：

- **SSID**：`XC-RID-XXXXXX`（后 6 位为 MAC 地址）
- **密码**：`12345678`
- **配置页面**：`http://192.168.4.1`

填写以下信息后保存，设备重启进入广播模式：

| 字段 | 说明 |
|------|------|
| 唯一产品识别码 | 机身出厂序列号，符合 GB/T 41300，最多 20 位 |
| 实名登记标志 | 民航局实名登记号码的**最后 8 位** |
| 运行类别 | 开放类 / 特定类 / 审定类 |
| 无人机分类 | 微型 / 轻型 / 小型 / 中型 / 大型 |
| UART 引脚 | 连接飞控 MAVLink 输出的 RX/TX 引脚 |
| 波特率 | ArduPilot 默认 57600，iNav 默认 115200 |

上电时按住配置入口键可重新进入配置模式。当前固件检测 GPIO9；ESP32-C3 开发板通常可直接使用板载 BOOT 键，ESP32-S3、ESP32 WROOM-32 等板型部署前需把配置入口引脚改到实际按键或外接按键。

### LED 状态

| 状态 | 指示 | 含义 |
|------|------|------|
| INIT | 蓝色慢闪 | 系统启动中 |
| CONFIG | 蓝色快闪 | 配置热点已开启 |
| WAIT_DATA | 黄色慢闪 | 已进入正常模式，等待飞控位置数据 |
| OK | 绿色常亮 | RID 数据有效，广播和联锁正常 |
| RID_FAIL | 红色快闪 | RID 输入、广播初始化、fllog 分区挂载或联锁检查失败 |
| ERROR | 红色常亮 | 保留错误状态 |

---

## 飞控接入

支持通过 MAVLink 接收飞控数据，兼容 **ArduPilot** 和 **iNav**。

### 接线示意

```
飞控 TELEM 口                    XC-RemoteID (ESP32)
┌─────────────┐                 ┌─────────────────┐
│         5V  ├────────────────►│ 5V              │
│        GND  ├────────────────►│ GND             │
│         TX  ├────────────────►│ RX              │
│         RX  │◄────────────────┤ TX              │
└─────────────┘                 └─────────────────┘
```

> RX 接 TX，TX 接 RX，交叉连接。

### 各板型默认引脚

| 板型 | RX 引脚 | TX 引脚 | 供电 |
|------|---------|---------|------|
| ESP32-S3 DevKitC-1 | GPIO17 | GPIO18 | 5V 或 3.3V |
| ESP32-C3 Super Mini | GPIO20 | GPIO21 | 5V 或 3.3V |
| ESP32-C3 DevKit | GPIO20 | GPIO21 | 5V 或 3.3V |
| XIAO ESP32-C3 | GPIO7 | GPIO6 | 5V 或 3.3V |
| ESP32 WROOM-32 | GPIO16 | GPIO17 | 5V 或 3.3V |

> 引脚可在配置页面修改，无需重新刷写固件。

### 飞控配置

飞控需开启 MAVLink 输出，并发送以下消息：
- `OPEN_DRONE_ID_LOCATION`（位置、速度、高度）
- `OPEN_DRONE_ID_SYSTEM`（遥控站位置）
- `OPEN_DRONE_ID_SYSTEM_UPDATE`（遥控站位置更新）
- `SYSTEM_TIME`（Unix 时间基准，用于还原 GB 时间戳）

**ArduPilot**：`SERIALx_PROTOCOL = 2`（MAVLink2），`SERIALx_BAUD = 57`

**iNav**：端口功能选 `MSP & MAVLink`，波特率 `115200`

> 若飞控不发送 `SYSTEM_TIME`，固件仍会广播位置等信息，但 GB 时间戳字段会置 0（未知）。

---

## 工作原理

```
飞控 MAVLink
  ├─ OPEN_DRONE_ID_LOCATION / SYSTEM / SYSTEM_UPDATE
  ├─ SYSTEM_TIME
  ▼
MAVLinkInput
  ▼
RIDData
  ├─ GB46750Encoder ──► BLE 5.0 Extended Advertising
  │                   └► Wi-Fi Beacon Vendor IE
  ├─ FlightLog ────────► LittleFS 滚动日志（10s 间隔）
  └─ Interlock ────────► MAVLink OPEN_DRONE_ID_ARM_STATUS
```

正常模式下固件每 1s 更新 BLE/Wi-Fi 广播，每 10s 写入一条飞行日志。位置失效时仍继续广播，位置字段编码为未知，运行状态标记为 RID_FAIL（紧急状态保留 EMERGENCY）。

---

## 故障排查

| 现象 | 可能原因 | 检查项 |
|------|----------|--------|
| 找不到配置热点 | 设备未进入配置模式 | 首次上电或按住配置入口键重启 |
| 配置后一直黄灯 | 未收到飞控位置数据 | 检查 UART RX/TX 交叉、波特率、MAVLink 输出 |
| 无法解锁 | RID 联锁失败 | 查看 LED 状态，确认身份参数、位置数据、广播初始化和 fllog 分区已挂载 |
| 时间戳为 0 | 缺少 `SYSTEM_TIME` | 配置飞控输出 `SYSTEM_TIME` |
| 接收端显示位置未知 | 飞控上报 0/0 或位置超时 | 检查 GPS 定位和 `OPEN_DRONE_ID_LOCATION` |
| 日志不可用 | fllog 分区挂载失败 | 检查分区表、Flash 状态和串口日志 |

---

## 从源码编译

```bash
cd XC-RemoteID
pio run -e esp32c3_supermini

# 编译并上传
pio run -e esp32c3_supermini --target upload
```

当前 `esp32c3_supermini` 环境编译通过。4MB Flash 的 app 分区空间较紧，后续加入网络式上报前需要关注固件体积或调整分区。

### 模拟数据模式（Mock）

无飞控时可使用 `esp32c3_supermini_mock` 环境验证完整链路。该模式跳过配置检查，直接进入 NORMAL 模式，使用内置模拟轨迹（上海青浦淀山湖附近绕圆飞行）驱动 BLE/Wi-Fi 广播、飞行日志写入和联锁逻辑。

**适用场景：**
- 首次上板验证 BLE/Wi-Fi 广播是否正常（用手机 RID 接收 App 扫描）
- 验证 LittleFS 存储分区挂载和日志写入
- 调试编码器输出格式，无需接飞控

**不适用场景：**
- 验证 MAVLink 输入解析
- 验证配置页面和参数存储
- 正式部署

```bash
# 编译并上传 mock 版本
pio run -e esp32c3_supermini_mock --target upload

# 查看串口日志（C3 USB CDC 模式）
pio device monitor -e esp32c3_supermini_mock
```

> Mock 模式下串口会输出 `[XC-RID] MOCK mode`、BLE 启动信息和存储状态。若看到 `[FlightLog] ready`，说明存储链路正常；若看到 `[BLE] BT5 ext adv started`，说明广播链路正常。

---

## 项目结构

```
src/
├── main.cpp                    # 主入口，两阶段启动
├── gb46750/
│   ├── fields.h                # 国标枚举和常量
│   └── encoder.cpp/.h          # GB 46750-2025 数据包编码器
├── transport/
│   └── mavlink_input.cpp/.h    # MAVLink 输入（ArduPilot / iNav）
├── broadcast/
│   ├── ble_tx.cpp/.h           # BLE 5.0 广播
│   └── wifi_tx.cpp/.h          # Wi-Fi Beacon 广播
├── storage/
│   └── flight_log.cpp/.h       # 145h 滚动存储（LittleFS，满足 §5.1.8 ≥120h）
├── system/
│   ├── parameters.cpp/.h       # NVS 参数存储
│   ├── led.cpp/.h              # LED 状态指示
│   └── interlock.cpp/.h        # 起飞联锁
├── webserver/
│   └── web_server.cpp/.h       # AP 热点配置页面
└── mock/
    └── mock_data.cpp/.h        # 模拟数据（-DMOCK_DATA 启用）
```

---

## License

GPL v2
