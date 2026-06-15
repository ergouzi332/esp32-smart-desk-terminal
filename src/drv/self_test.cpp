#include "drv/self_test.h"
#include "bsp/board.h"
#include "drv/dht11_drv.h"
#include "drv/max30102_drv.h"
#include "drv/battery_drv.h"
#include <Arduino.h>
#include <WiFi.h>

SelfTestResult_t g_selfTest;

void SelfTest_RunAll(void) {
    /* OLED - Board_Init() 成功后就能操作，永远 true */
    g_selfTest.oled = true;

    /* DHT11 - 尝试读取一次，检查值是否在合理范围 */
    float t = dht11.readTemperature();
    float h = dht11.readHumidity();
    g_selfTest.dht11 = !isnan(t) && !isnan(h) && t > -20.0f && t < 60.0f;

    /* 电池 - 读取 ADC，判断百分比是否在合理范围 */
    GetCur_Power();
    g_selfTest.battery = (CurBattery > 0 && CurBattery <= 100);

    /* MAX30102 - 查询初始化时 I2C 检测结果 */
    g_selfTest.max30102 = max30102_is_ready();

    /* SU03T 语音模块 - 无源检测，默认通过 */
    g_selfTest.voice = true;

    /* WiFi - 检查硬件是否就绪（不等连接完成） */
    g_selfTest.wifi = (WiFi.status() != WL_NO_SHIELD);
}

void SelfTest_ShowScreen(void) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);

    /* 标题 */
    u8g2.setCursor(0, 10);
    u8g2.print("POST:");

    /* 第一行：OLED / DHT11 */
    u8g2.setCursor(0, 23);
    u8g2.print(g_selfTest.oled ? "[OK] OLED" : "[--] OLED");
    u8g2.setCursor(68, 23);
    u8g2.print(g_selfTest.dht11 ? "[OK] DHT11" : "[--] DHT11");

    /* 第二行：BAT / MAX301 */
    u8g2.setCursor(0, 35);
    u8g2.print(g_selfTest.battery ? "[OK] BAT" : "[--] BAT");
    u8g2.setCursor(68, 35);
    u8g2.print(g_selfTest.max30102 ? "[OK] MAX301" : "[--] MAX301");

    /* 第三行：VOICE / WIFI */
    u8g2.setCursor(0, 47);
    u8g2.print(g_selfTest.voice ? "[OK] SU03T" : "[--] SU03T");
    u8g2.setCursor(68, 47);
    u8g2.print(g_selfTest.wifi ? "[OK] WIFI" : "[--] WIFI");

    u8g2.sendBuffer();

    /* 显示 2 秒后状态机接管 */
    delay(2000);
}
