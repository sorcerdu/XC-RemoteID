/*
 * Web 配置服务器（AP 热点模式）
 * 用户通过连接热点访问 192.168.4.1 进行配置
 */
#pragma once

class XCWebServer {
public:
    void start_ap();
    void update();
    void stop();

private:
    bool _started = false;

    void handle_root();
    void handle_save();
    void handle_status();
    void handle_reboot();

    static const char *AP_SSID;
    static const char *AP_PASS;
};
