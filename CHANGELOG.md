# Changelog

本文件记录每个版本的具体变更。版本号遵循 [SemVer](https://semver.org/lang/zh-CN/)。

## v0.1.1

### Bug 修复

- **Wi-Fi 广播**：Vendor IE 封装补齐 message counter 字节，对齐 BLE 链路结构（`[OUI_TYPE][counter][GB 包]`）。修复前缺少 counter，导致 ASTM 兼容接收端解析时把首个 GB 字节当 counter，整包字节错位无法识别。

## v0.1.0

首个广播式运行识别测试版本。

### 已实现

- BLE 5.0 Extended Advertising 广播（ESP32-C3 / S3）
- Wi-Fi Beacon Vendor IE 广播
- GB 46750-2025 数据包编码（21 个数据内容项）
- MAVLink 输入（兼容 ArduPilot / iNav）
- LittleFS 滚动存储（≈145h，满足 §5.1.8 ≥120h 要求）
- 起飞联锁（`OPEN_DRONE_ID_ARM_STATUS`）
- AP 热点 + Web 配置页面
- 五板型支持：ESP32-S3 DevKitC-1、ESP32-C3 Super Mini、ESP32-C3 DevKit、XIAO ESP32-C3、ESP32 WROOM-32

### 已知限制

- 网络式上报未实现（待按民航局 2026-03-06 公告对接 UOM）
- 防篡改/防破坏设计未实现（依赖量产硬件 eFuse 锁定）
- 时间戳精度依赖飞控发送 `SYSTEM_TIME` 消息
