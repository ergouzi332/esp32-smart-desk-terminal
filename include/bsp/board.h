#ifndef BSP_BOARD_H
#define BSP_BOARD_H

#include <Arduino.h>
#include <U8g2lib.h>

/*SU03T语音模块串口引脚*/
#define SU03T_RX_PIN 41
#define SU03T_TX_PIN 42

/*OLED的I2C总线引脚*/
#define I2C_SDA 21
#define I2C_SCL 20

/*DHT温湿度传感器引脚*/
#define DHT_PIN 18

/*电池电压检测ADC引脚*/
#define BATTERY_PIN 4

/*MAX30102心率血氧传感器I2C引脚*/
#define MAX30102_I2C_SDA 16
#define MAX30102_I2C_SCL 15

extern HardwareSerial SerialSU03T;   // SU03T语音模块串口对象
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;  // OLED显示屏对象

/*板级硬件初始化*/
void Board_Init();

#endif