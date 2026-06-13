#ifndef APP_APP_DATA_H
#define APP_APP_DATA_H
#include <Arduino.h>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

/*系统显示状态枚举*/
typedef enum {
    STATE_IDLE,         // 空闲状态
    STATE_SHOW_TIME,    // 显示时间
    STATE_SHOW_WEATHER, // 显示天气
    STATE_SHOW_HR,      // 实时采集心率
    STATE_SHOW_HR_RESULT,// 显示心率结果
    STATE_SHOW_ENV,     // 显示温湿度环境数据
    STATE_SHOW_BATTERY  // 显示电池电量
} SystemState;

extern SystemState currentState;    // 当前系统状态
extern SystemState lastState;       // 上一次系统状态

extern int hrSampleCount;           // 心率采样次数
extern int hrSampleSum;             // 心率采样累加值
extern int hrFinalAverage;          // 最终平均心率

/*传感器综合数据结构体*/
typedef struct {
    float temperature;
    float humidity;
    uint16_t batteryPercent;
    bool valid;
} SensorData_t;

extern QueueHandle_t voiceQueue;        // 语音消息队列句柄
extern QueueHandle_t sensorQueue;       // 传感器数据队列句柄
extern SemaphoreHandle_t sharedDataMutex; // 共享数据互斥信号量

/*获取系统运行毫秒数*/
uint32_t getTickMs(void);

/*锁定共享数据（进入临界区）*/
void lockSharedData(void);

/*解锁共享数据（退出临界区）*/
void unlockSharedData(void);

/*更新最新传感器数据*/
void setLatestSensorData(const SensorData_t &data);

/* 获取一份最新传感器数据副本 */
SensorData_t getLatestSensorDataCopy(void);

#endif