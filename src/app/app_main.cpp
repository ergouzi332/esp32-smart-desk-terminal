#include "app/app_main.h"
#include "app/app_data.h"
#include "app/state_machine.h"
#include "bsp/board.h"
#include "drv/wifi_drv.h"
#include "drv/dht11_drv.h"
#include "drv/battery_drv.h"
#include "drv/time_drv.h"

/* 应用初始化：创建 FreeRTOS 任务与 IPC 对象 */
void App_Init(void)
{
    sharedDataMutex = xSemaphoreCreateMutex();
    if (sharedDataMutex == NULL) { while (1) { } }
    voiceQueue = xQueueCreate(8, sizeof(uint8_t));
    sensorQueue = xQueueCreate(1, sizeof(SensorData_t));

    // WiFi 状态检测：500ms
    xTaskCreatePinnedToCore([](void* p) 
    { for (;;) 
        { wifi_task_loop(); 
            vTaskDelay(pdMS_TO_TICKS(500)); 
        } 
    }, 
    "wifiTask", 4096, NULL, 1, NULL, 1);

    // 语音监听：串口读取 SU-03T 指令
    xTaskCreatePinnedToCore([](void* p) 
    { for (;;) 
        { if (Serial.available()) 
            { uint8_t c = Serial.read(); 
                xQueueSend(voiceQueue, &c, 0); 
            } 
            vTaskDelay(pdMS_TO_TICKS(10)); 
        } 
    }, 
    "voiceTask", 4096, NULL, 3, NULL, 0);

    // 传感器采集：2s 温湿度/电量
    xTaskCreatePinnedToCore([](void* p) 
    { SensorData_t sd = {0,0,0,false}; 
    getTime(); 
    for (;;) 
    { sd.temperature = dht11.readTemperature(); 
      sd.humidity = dht11.readHumidity(); 
      GetCur_Power(); 
      sd.batteryPercent = CurBattery; 
      sd.valid = true; 
      xQueueOverwrite(sensorQueue, &sd);
      vTaskDelay(pdMS_TO_TICKS(2000)); 
    } 
    }, 
    "sensorTask", 4096, NULL, 2, NULL, 0);

    // 状态机：核心编排，20ms 循环
    xTaskCreatePinnedToCore([](void* p) 
    { uint8_t c; 
        SensorData_t nd; 
        for (;;) 
        { if (xQueueReceive(voiceQueue, &c, 0) == pdPASS) 
            { if (c==0xAA) currentState=STATE_SHOW_TIME; 
                else if (c==0xBB) currentState=STATE_SHOW_WEATHER; 
                else if (c==0xCC) currentState=STATE_SHOW_HR; 
                else if (c==0xDD) currentState=STATE_SHOW_ENV; 
                else if (c==0xEE) currentState=STATE_SHOW_BATTERY; 
            } 
            if (xQueueReceive(sensorQueue, &nd, 0) == pdPASS) 
            { 
                setLatestSensorData(nd); 
                SystemState prev = currentState; 
                runStateMachine(); 
                lastState = prev; 
                vTaskDelay(pdMS_TO_TICKS(20)); 
            } 
        } 
    }, "stateMachineTask", 8192, NULL, 2, NULL, 1);
}
