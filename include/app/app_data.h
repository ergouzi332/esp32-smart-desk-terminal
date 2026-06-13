#ifndef APP_APP_DATA_H
#define APP_APP_DATA_H
#include <Arduino.h>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
typedef enum { STATE_IDLE, STATE_SHOW_TIME, STATE_SHOW_WEATHER, STATE_SHOW_HR, STATE_SHOW_HR_RESULT, STATE_SHOW_ENV, STATE_SHOW_BATTERY } SystemState;
extern SystemState currentState;
extern SystemState lastState;
extern int hrSampleCount;
extern int hrSampleSum;
extern int hrFinalAverage;
typedef struct { float temperature; float humidity; uint16_t batteryPercent; bool valid; } SensorData_t;
extern QueueHandle_t voiceQueue;
extern QueueHandle_t sensorQueue;
extern SemaphoreHandle_t sharedDataMutex;
uint32_t getTickMs(void);
void lockSharedData(void);
void unlockSharedData(void);
void setLatestSensorData(const SensorData_t &data);
SensorData_t getLatestSensorDataCopy(void);
#endif
