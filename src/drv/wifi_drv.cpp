#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "drv/wifi_drv.h"

uint8_t wifi_connected = 0;

// WiFi 连接状态检测
// WiFi 连接状态检测
void wifi_task_loop(void)
{
    if (WiFi.status() == WL_CONNECTED) wifi_connected = 1;
    else wifi_connected = 0;
}

// 启动 WiFi（SSID/密码在 config.h）
// 启动 WiFi
void wifi_connect_start(void)
{
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}
