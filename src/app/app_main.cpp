#include "app/app_main.h"
#include "app/app_data.h"
#include "app/state_machine.h"
#include "bsp/board.h"
#include "drv/wifi_drv.h"
#include "drv/dht11_drv.h"
#include "drv/battery_drv.h"
#include "drv/time_drv.h"

void App_Init(void)
{
    sharedDataMutex = xSemaphoreCreateMutex();
    if (sharedDataMutex == NULL) {
        while (1) { }
    }

    voiceQueue = xQueueCreate(8, sizeof(uint8_t));
    sensorQueue = xQueueCreate(1, sizeof(SensorData_t));

    xTaskCreatePinnedToCore(
        [](void* param) {
            for (;;) {
                wifi_task_loop();
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        },
        "wifiTask", 4096, NULL, 1, NULL, 1);

    xTaskCreatePinnedToCore(
        [](void* param) {
            for (;;) {
                if (Serial.available()) {
                    uint8_t cmd = Serial.read();
                    xQueueSend(voiceQueue, &cmd, 0);
                }
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        },
        "voiceTask", 4096, NULL, 3, NULL, 0);

    xTaskCreatePinnedToCore(
        [](void* param) {
            SensorData_t sensorData = {0.0f, 0.0f, 0, false};
            int timeRefreshCnt = 0;
            getTime();
            for (;;) {
                sensorData.temperature = dht11.readTemperature();
                sensorData.humidity = dht11.readHumidity();
                GetCur_Power();
                sensorData.batteryPercent = CurBattery;
                sensorData.valid = true;
                xQueueOverwrite(sensorQueue, &sensorData);
                if (++timeRefreshCnt >= 15) {
                    timeRefreshCnt = 0;
                    getTime();
                }
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
        },
        "sensorTask", 4096, NULL, 2, NULL, 0);

    xTaskCreatePinnedToCore(
        [](void* param) {
            uint8_t cmd;
            SensorData_t newSensorData;
            for (;;) {
                if (xQueueReceive(voiceQueue, &cmd, 0) == pdPASS) {
                    if (cmd == 0xAA) currentState = STATE_SHOW_TIME;
                    else if (cmd == 0xBB) currentState = STATE_SHOW_WEATHER;
                    else if (cmd == 0xCC) currentState = STATE_SHOW_HR;
                    else if (cmd == 0xDD) currentState = STATE_SHOW_ENV;
                    else if (cmd == 0xEE) currentState = STATE_SHOW_BATTERY;
                }
                if (xQueueReceive(sensorQueue, &newSensorData, 0) == pdPASS) {
                    setLatestSensorData(newSensorData);
                }
                SystemState prevState = currentState;
                runStateMachine();
                lastState = prevState;
                vTaskDelay(pdMS_TO_TICKS(20));
            }
        },
        "stateMachineTask", 8192, NULL, 2, NULL, 1);
}
