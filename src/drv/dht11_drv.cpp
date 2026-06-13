#include "bsp/board.h"
#include "drv/dht11_drv.h"
#include <Arduino.h>
#include <DHT.h>

#define DHT_TYPE DHT11
DHT11_Typedef DHT11_State;
DHT dht11(DHT_PIN, DHT_TYPE);

// DHT11 温湿度传感器初始化
// DHT11 初始化
void DHT11_Init(void)
{
    dht11.begin();
}

// 读取 DHT11 数据到全局状态
// 读取 DHT11 温湿度
void readDHT11(void)
{
    DHT11_State.temperature = dht11.readTemperature();
    DHT11_State.humidity = dht11.readHumidity();
}
