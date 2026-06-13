#include "bsp/board.h"
#include <Arduino.h>
#include "drv/battery_drv.h"

uint16_t CurBattery = 0;
static uint32_t temp1 = 0;
static uint8_t Battery_num = 0;

/* 电池分压检测：4颗2k电阻串联，分压比 1/4 */
/* 锂电池范围 3.0V~4.2V，ADC电压 = 电池电压 × 0.25 */
/* 公式：(3.3 × 4 × ADC / 4096) × 100 - 300 = 0~120，映射到 0~100% */
void GetBattery_Init(void)
{
    analogReadResolution(12);
}

/* 获取当前电池电量（100次采样平均滤波）*/
void GetCur_Power(void)
{
    if (++Battery_num <= 100) {
        float v = (3.3f * 4.0f * analogRead(BATTERY_PIN) / 4096.0f) * 100.0f - 300.0f;
        if (v < 0) v = 0;
        if (v > 120) v = 100;
        temp1 += (uint16_t)(v * 100.0f / 120.0f);
    } else {
        CurBattery = (uint16_t)(temp1 / 100);
        temp1 = 0;
        Battery_num = 0;
    }
}
