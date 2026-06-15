#ifndef DRV_SELF_TEST_H
#define DRV_SELF_TEST_H

#include <stdint.h>
#include <stdbool.h>

/* 开机自检结果结构体 */
typedef struct {
    bool oled;       /* OLED 显示屏 */
    bool dht11;      /* DHT11 温湿度传感器 */
    bool battery;    /* 电池电压检测 */
    bool max30102;   /* MAX30102 心率血氧 */
    bool voice;      /* SU03T 语音模块 */
    bool wifi;       /* WiFi 模块 */
} SelfTestResult_t;

/* 全局自检结果，可供其他模块读取 */
extern SelfTestResult_t g_selfTest;

/** 执行所有自检项 */
void SelfTest_RunAll(void);

/** 在 OLED 上显示自检结果（显示 2 秒后自动清除） */
void SelfTest_ShowScreen(void);

#endif /* DRV_SELF_TEST_H */
