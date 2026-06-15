#include <Arduino.h>
#include <Wire.h>
#include "bsp/board.h"
#include "drv/wifi_drv.h"
#include "drv/dht11_drv.h"
#include "drv/max30102_drv.h"
#include "drv/battery_drv.h"
#include "app/app_main.h"
 #include "drv/self_test.h"

/* 系统初始化 */
void setup()
{
    Board_Init();
    Serial.begin(115200);
    SerialSU03T.begin(9600, SERIAL_8N1, SU03T_RX_PIN, SU03T_TX_PIN);
    wifi_connect_start();
    setenv("TZ", "CST-8", 1);
    tzset();
    DHT11_Init();
    MAX30102_Init();
    GetBattery_Init();
 
     /* 开机自检 */
     SelfTest_RunAll();
     SelfTest_ShowScreen();
 
    App_Init();
}

/* 主循环：FreeRTOS 已接管全部逻辑 */
void loop()
{
    vTaskDelay(pdMS_TO_TICKS(10));
}
