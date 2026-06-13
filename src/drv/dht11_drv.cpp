#include "bsp/board.h"
#include "drv/dht11_drv.h"
#include <Arduino.h>
#include <DHT.h>

#define DHT_TYPE DHT11
DHT11_Typedef DHT11_State;
DHT dht11(DHT_PIN, DHT_TYPE);

void DHT11_Init(void)
{
    dht11.begin();
}

void readDHT11(void)
{
    DHT11_State.temperature = dht11.readTemperature();
    DHT11_State.humidity = dht11.readHumidity();
}
