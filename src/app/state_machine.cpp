#include <Arduino.h>
#include "app/app_data.h"
#include "app/state_machine.h"
#include "bsp/board.h"
#include "drv/display_drv.h"
#include "drv/weather_drv.h"
#include "drv/time_drv.h"
#include "drv/max30102_drv.h"
#include "drv/wifi_drv.h"

void runStateMachine()
{
    switch (currentState)
    {
                    // 空闲：WiFi 已连每 700ms 闪文字，未连每 1s 刷连接画面
            case STATE_IDLE:
        {
            static unsigned long blinkTimer;
            static bool showText = true;
            static unsigned long idleRefresh;
            if (currentState != lastState)
            {
                blinkTimer = getTickMs();
                idleRefresh = getTickMs();
                showText = true;
                u8g2.clearBuffer();
            }
            if (wifi_connected)
            {
                if (getTickMs() - blinkTimer >= 700)
                {
                    blinkTimer = getTickMs();
                    showText = !showText;
                    u8g2.clearBuffer();
                    if (showText) drawWifiConnected2();
                    else drawWifiConnected1();
                    u8g2.sendBuffer();
                }
            } else
            {
                if (getTickMs() - idleRefresh >= 1000)
                {
                    idleRefresh = getTickMs();
                    u8g2.clearBuffer();
                    drawWifiConnecting1();
                    u8g2.sendBuffer();
                }
            }
            break;
        }
                    // 显示时间：读取 Time_State 缓存，4 秒后回空闲
            case STATE_SHOW_TIME:
        {
            static unsigned long start_time_time;
            if (currentState != lastState)
            {
                start_time_time = getTickMs();
                if (wifi_connected)
                {
                    u8g2.clearBuffer();
                    if (Time_State.year == 0) getTime();
                    drawTime();
                    u8g2.sendBuffer();
                } else
                {
                    u8g2.clearBuffer();
                    drawPreaseConnect();
                    drawWifiConnecting2();
                    u8g2.sendBuffer();
                }
            }
            if (getTickMs() - start_time_time >= 4000)
            {
                currentState = STATE_IDLE;
            }
            break;
        }
                    // 显示天气：HTTP 获取数据，4 秒后回空闲
            case STATE_SHOW_WEATHER:
        {
            static unsigned long start_time_weather;
            if (currentState != lastState)
            {
                start_time_weather = getTickMs();
                if (wifi_connected)
                {
                    u8g2.clearBuffer();
                    getWeather();
                    drawWeather();
                    u8g2.sendBuffer();
                } else
                {
                    u8g2.clearBuffer();
                    drawPreaseConnect();
                    drawWifiConnecting2();
                    u8g2.sendBuffer();
                }
            }
            if (getTickMs() - start_time_weather >= 4000)
            {
                currentState = STATE_IDLE;
            }
            break;
        }
                    // 心率测量：10 秒采样 MAX30102，测量完进结果状态
            case STATE_SHOW_HR:
        {
            static unsigned long start_time_hr;
            if (currentState != lastState)
            {
                start_time_hr = getTickMs();
                hrSampleCount = 0;
                hrSampleSum = 0;
                hrFinalAverage = 0;
                readMAX30102();
                u8g2.clearBuffer();
                drawHeartSpO2(10, false, 0);
                u8g2.sendBuffer();
            }
            readMAX30102();
            if (MAX30102_State.valid && MAX30102_State.heartRate > 0)
            {
                hrSampleCount++;
                hrSampleSum += MAX30102_State.heartRate;
            }
            unsigned long elapsed = getTickMs() - start_time_hr;
            int remaining = 10 - (int)(elapsed / 1000);
            if (remaining < 0) remaining = 0;
            if (wifi_connected)
            {
                u8g2.clearBuffer();
                drawHeartSpO2(remaining, false, 0);
                u8g2.sendBuffer();
            } else
            {
                u8g2.clearBuffer();
                drawHeartSpO2(remaining, false, 0);
                drawWifiConnecting2();
                u8g2.sendBuffer();
            }
            if (elapsed >= 10000)
            {
                if (hrSampleCount > 0) hrFinalAverage = (hrSampleSum + hrSampleCount / 2) / hrSampleCount;
                else hrFinalAverage = MAX30102_State.heartRate;
                currentState = STATE_SHOW_HR_RESULT;
            }
            break;
        }
                    // 心率结果：显示平均 BPM，2 秒后回空闲
            case STATE_SHOW_HR_RESULT:
        {
            static unsigned long result_start_ms;
            static int displayValue;
            if (currentState != lastState)
            {
                result_start_ms = getTickMs();
                displayValue = hrFinalAverage > 0 ? hrFinalAverage : 0;
            }
            unsigned long elapsed = getTickMs() - result_start_ms;
            bool showRate = ((elapsed / 500) % 2) == 0;
            if (wifi_connected)
            {
                u8g2.clearBuffer();
                drawHeartSpO2(0, true, displayValue, showRate);
                u8g2.sendBuffer();
            } else
            {
                u8g2.clearBuffer();
                drawHeartSpO2(0, true, displayValue, showRate);
                drawWifiConnecting2();
                u8g2.sendBuffer();
            }
            if (elapsed >= 2000)
            {
                currentState = STATE_IDLE;
            }
            break;
        }
                    // 温湿度：显示缓存传感器数据，4 秒后回空闲
            case STATE_SHOW_ENV:
        {
            static unsigned long start_time_dth11;
            if (currentState != lastState)
            {
                start_time_dth11 = getTickMs();
                SensorData_t cachedSensorData = getLatestSensorDataCopy();
                if (wifi_connected)
                {
                    u8g2.clearBuffer();
                    drawDTH11();
                    u8g2.sendBuffer();
                } else
                {
                    u8g2.clearBuffer();
                    drawDTH11();
                    drawWifiConnecting2();
                    u8g2.sendBuffer();
                }
            }
            if (getTickMs() - start_time_dth11 >= 4000) currentState = STATE_IDLE;
            break;
        }
                    // 电量：显示缓存电池百分比，4 秒后回空闲
            case STATE_SHOW_BATTERY:
        {
            static unsigned long start_time_battery;
            if (currentState != lastState) {
                start_time_battery = getTickMs();
                u8g2.clearBuffer();
                SensorData_t cachedSensorData = getLatestSensorDataCopy();
                if (wifi_connected)
                {
                    drawBatteryLevel();
                    u8g2.sendBuffer();
                } else
                {
                    drawBatteryLevel();
                    drawWifiConnecting2();
                    u8g2.sendBuffer();
                }
            }
            if (getTickMs() - start_time_battery >= 4000) currentState = STATE_IDLE;
            break;
        }
    }
}
