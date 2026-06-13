#include "bsp/board.h"

/* SU-03T 语音串口（UART2）*/
HardwareSerial SerialSU03T(2);
/* OLED（硬件 I2C, SCL=20, SDA=21）*/
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, I2C_SCL, I2C_SDA);

void Board_Init()
{
    u8g2.begin();
    u8g2.setBusClock(400000);
    u8g2.clearBuffer();
}
