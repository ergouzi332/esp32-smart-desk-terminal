#ifndef BSP_BOARD_H
#define BSP_BOARD_H
#include <Arduino.h>
#include <U8g2lib.h>
#define SU03T_RX_PIN 41
#define SU03T_TX_PIN 42
#define I2C_SDA 21
#define I2C_SCL 20
#define DHT_PIN 18
#define BATTERY_PIN 4
#define MAX30102_I2C_SDA 16
#define MAX30102_I2C_SCL 15
extern HardwareSerial SerialSU03T;
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
void Board_Init();
#endif
