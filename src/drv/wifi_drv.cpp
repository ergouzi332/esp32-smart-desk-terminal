#include <Arduino.h>
#include "drv/time_drv.h"
#include <WiFi.h>
#include "config.h"
#include "drv/wifi_drv.h"

uint8_t wifi_connected = 0;

// WiFi 连接状态检测，首次连上时自动校准 RTC
void wifi_task_loop(void)
{
    static bool timeFetched = false;
    if (WiFi.status() == WL_CONNECTED) {
        wifi_connected = 1;
        if (!timeFetched) {
            timeFetched = true;
            getTime(); // 首次连上 WiFi 时校准 RTC
        }
    } else {
        wifi_connected = 0;
    }
}

void wifi_connect_start(void)
{
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}
