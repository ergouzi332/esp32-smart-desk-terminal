#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "drv/time_drv.h"
#include <time.h>

Time_Typedef Time_State;
long long currentTime;

void getTime()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        String url = "https://f.m.suning.com/api/ct.do";
        http.begin(url);
        int httpCode = http.GET();
        if (httpCode > 0)
        {
            String payload = http.getString();
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (error)
            {
                Serial.print("JSON parse failed: ");
                Serial.println(error.f_str());
                return;
            }
            currentTime = doc["currentTime"].as<long long>();
            struct tm timeinfo;
            time_t now = currentTime / 1000;
            localtime_r(&now, &timeinfo);
            Time_State.year = timeinfo.tm_year + 1900;
            Time_State.month = timeinfo.tm_mon + 1;
            Time_State.day = timeinfo.tm_mday;
            Time_State.hour = timeinfo.tm_hour;
            Time_State.minute = timeinfo.tm_min;
        }
        else
        {
            Serial.print("HTTP request failed: ");
            Serial.println(http.errorToString(httpCode).c_str());
        }
        http.end();
    }
    else
    {
        Serial.println("WiFi not connected");
    }
}
