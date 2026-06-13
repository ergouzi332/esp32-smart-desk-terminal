#include "app/app_data.h"

SystemState currentState = STATE_IDLE;
SystemState lastState = STATE_SHOW_WEATHER;

int hrSampleCount = 0;
int hrSampleSum = 0;
int hrFinalAverage = 0;

QueueHandle_t voiceQueue = NULL;
QueueHandle_t sensorQueue = NULL;
static SensorData_t latestSensorData = {0.0f, 0.0f, 0, false};
SemaphoreHandle_t sharedDataMutex = NULL;

uint32_t getTickMs(void)
{
    return pdTICKS_TO_MS(xTaskGetTickCount());
}

void lockSharedData(void)
{
    xSemaphoreTake(sharedDataMutex, portMAX_DELAY);
}

void unlockSharedData(void)
{
    xSemaphoreGive(sharedDataMutex);
}

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
