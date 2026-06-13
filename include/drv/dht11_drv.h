#ifndef DRV_DHT11_DRV_H
#define DRV_DHT11_DRV_H

#include <DHT.h>

/*DHT11温湿度数据结构体*/
typedef struct {
    float temperature;
    float humidity;
} DHT11_Typedef;

extern DHT11_Typedef DHT11_State;  // DHT11传感器状态数据
extern DHT dht11;                  // DHT11传感器操作对象

/*DHT11传感器初始化*/
void DHT11_Init(void);

/*读取DHT11温湿度数据*/
void readDHT11(void);

#endif