#ifndef DRV_DHT11_DRV_H
#define DRV_DHT11_DRV_H
#include <DHT.h>
typedef struct { float temperature; float humidity; } DHT11_Typedef;
extern DHT11_Typedef DHT11_State;
extern DHT dht11;
void DHT11_Init(void);
void readDHT11(void);
#endif
