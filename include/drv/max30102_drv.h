#ifndef DRV_MAX30102_DRV_H
#define DRV_MAX30102_DRV_H

#include <Arduino.h>

#define MAX30102_I2C_ADDR 0x57   //MAX30102的I2C地址
/*MAX30102心率血氧数据结构体*/
typedef struct
{ int heartRate; 
    int spo2; 
    bool valid; 
} MAX30102_Typedef;
extern MAX30102_Typedef MAX30102_State;

void MAX30102_Init(void);
void readMAX30102(void);
/** 检测 MAX30102 芯片是否在 I2C 总线上就绪 */
bool max30102_is_ready(void);
#endif
