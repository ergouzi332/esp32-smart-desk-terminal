#ifndef DRV_MAX30102_DRV_H
#define DRV_MAX30102_DRV_H
#include <Arduino.h>
#define MAX30102_I2C_ADDR 0x57
typedef struct { int heartRate; int spo2; bool valid; } MAX30102_Typedef;
extern MAX30102_Typedef MAX30102_State;
void MAX30102_Init(void);
void readMAX30102(void);
#endif
