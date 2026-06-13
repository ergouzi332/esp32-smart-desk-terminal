#include "bsp/board.h"
#include <Arduino.h>
#include "drv/battery_drv.h"

uint16_t CurBattery = 0;

void GetBattery_Init(void)
{
    pinMode(BATTERY_PIN, INPUT);
}

void GetCur_Power(void)
{
    int adcValue = analogRead(BATTERY_PIN);
    CurBattery = (uint16_t)map(adcValue, 0, 4095, 0, 100);
}
