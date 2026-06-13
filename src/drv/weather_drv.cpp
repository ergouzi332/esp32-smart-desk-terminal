#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "config.h"
#include "drv/weather_drv.h"

Weather_Typedef Weather_State;

void getWeather()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        String url = "http://api.seniverse.com/v3/weather/now.json?key=" + String(WEATHER_API_KEY) + "&location=" + String(WEATHER_CITY) + "&language=zh-Hans&unit=c";
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
            JsonObject now = doc["results"][0]["now"];
            JsonObject location = doc["results"][0]["location"];
            strcpy(Weather_State.cityName, location["name"]);
            strcpy(Weather_State.weatherText, now["text"]);
            Weather_State.temperature = now["temperature"].as<int>();
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
