#ifndef DRV_DISPLAY_DRV_H
#define DRV_DISPLAY_DRV_H

#include <U8g2lib.h>

void Display_Init();

void drawWifiConnected1();
void drawWifiConnected2();
void drawWifiConnecting1();
void drawWifiConnecting2();
void drawWeather();
void drawTime();
void drawDTH11();
void drawPreaseConnect();
void drawHeartSpO2(int countdownSeconds, bool showFinal, int animatedHeartRate, bool showHeartRate = true);
void drawBatteryLevel();

#endif
