#include "app/app_data.h"

/*当前系统状态和上一个系统状态*/
SystemState currentState = STATE_IDLE;
SystemState lastState = STATE_SHOW_WEATHER;

/*心率采样计数*/
int hrSampleCount = 0;
/*心率采样值累加*/
int hrSampleSum = 0;
/*心率最终平均值*/
int hrFinalAverage = 0;

/*语音命令队列*/
QueueHandle_t voiceQueue = NULL;
/*传感器数据队列（覆盖写入）*/
QueueHandle_t sensorQueue = NULL;
/* 传感器数据缓存（互斥锁保护）*/
static SensorData_t latestSensorData = {0.0f, 0.0f, 0, false};
/* 共享数据互斥锁 */
SemaphoreHandle_t sharedDataMutex = NULL;

/*获取系统滴答时间（ms）*/
uint32_t getTickMs(void)
{
    return pdTICKS_TO_MS(xTaskGetTickCount());
}

/* 加锁 */
void lockSharedData(void) { xSemaphoreTake(sharedDataMutex, portMAX_DELAY); }
/* 解锁 */
void unlockSharedData(void) { xSemaphoreGive(sharedDataMutex); }

void setLatestSensorData(const SensorData_t &data)
{
    lockSharedData();
    latestSensorData = data;
    unlockSharedData();
}

SensorData_t getLatestSensorDataCopy(void)
{
    SensorData_t data;
    lockSharedData();
    data = latestSensorData;
    unlockSharedData();
    return data;
}
