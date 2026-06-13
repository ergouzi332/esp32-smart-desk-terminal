#ifndef DRV_BATTERY_DRV_H
#define DRV_BATTERY_DRV_H
#include <stdint.h>
void GetBattery_Init(void);
void GetCur_Power(void);
extern uint16_t CurBattery;
#endif
