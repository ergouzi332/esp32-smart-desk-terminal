#include "app/app_main.h"
#include "app/app_data.h"
#include "app/state_machine.h"
#include "bsp/board.h"
#include "drv/wifi_drv.h"
#include "drv/dht11_drv.h"
#include "drv/battery_drv.h"
#include "drv/time_drv.h"
#include "app/ota.h"
#include "app/fault_handler.h"
#include "app/stack_monitor.h"
#include <Preferences.h>
#include <esp_ota_ops.h>

#define OTA_MAX_BOOT_ATTEMPTS 4

void App_Init(void)
{
    /* === 修正OTA引导分区：确保USB烧录后从当前运行分区启动 === */
    {
        const esp_partition_t *running = esp_ota_get_running_partition();
        const esp_partition_t *boot = esp_ota_get_boot_partition();
        if (running != NULL && boot != NULL && running != boot) {
            Serial.printf("[BOOT] 引导分区(%s)与运行分区(%s) 不匹配，正在修正\n",
                          boot->label, running->label);
            esp_ota_set_boot_partition(running);
            Serial.println("[BOOT] 重启后生效");
        }
    }

    /* === 故障检测：先于一切，检测上次复位原因 === */
    fault_handler_init();
    if (fault_handler_check_boot()) {
        fault_handler_print_info();
    }

    /* === OTA 回滚检测：检测新固件崩溃循环 === */

    {
        Preferences pref;
        pref.begin("ota", false);
        bool pending = pref.getBool("pending", false);
        if (pending) {
            uint8_t attempt = pref.getUChar("attempt", 0) + 1;
            pref.putUChar("attempt", attempt);
            pref.end();
            Serial.printf("[OTA] 启动尝试 %d/%d\n", attempt, OTA_MAX_BOOT_ATTEMPTS);
            if (attempt >= OTA_MAX_BOOT_ATTEMPTS) {
                const esp_partition_t *part = esp_ota_get_next_update_partition(NULL);
                if (part) {
                    esp_ota_set_boot_partition(part);
                    Serial.println("[OTA] 启动失败次数过多，回滚到旧分区");
                    delay(100);
                    esp_restart();
                }
            }
        } else {
            pref.end();
        }
    }



    sharedDataMutex = xSemaphoreCreateMutex();

    if (sharedDataMutex == NULL) { while (1) { } }
    voiceQueue = xQueueCreate(8, sizeof(uint8_t));
    sensorQueue = xQueueCreate(1, sizeof(SensorData_t));


    /* WiFi 状态检测：500ms */
    xTaskCreatePinnedToCore([](void* p) 
    { for (;;) 
        { wifi_task_loop(); 
            vTaskDelay(pdMS_TO_TICKS(500)); 
        } 
    }, 
    "wifiTask", 4096, NULL, 1, NULL, 1);

    /* 语音监听：串口读取 SU-03T 指令 */
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

    /* 传感器采集：2s 温湿度/电量 */
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

    /* 状态机：核心编排，20ms 循环 */
    xTaskCreatePinnedToCore([](void* p) 
    { uint8_t c; 
        SensorData_t nd; 
        for (;;) 
        { 
            if (xQueueReceive(voiceQueue, &c, 0) == pdPASS) 
            { 
                if (c==0xAA) currentState=STATE_SHOW_TIME; 
                else if (c==0xBB) currentState=STATE_SHOW_WEATHER; 
                else if (c==0xCC) currentState=STATE_SHOW_HR; 
                else if (c==0xDD) currentState=STATE_SHOW_ENV; 
                else if (c==0xEE) currentState=STATE_SHOW_BATTERY; 
            } 
            if (xQueueReceive(sensorQueue, &nd, 0) == pdPASS) 
            { 
                setLatestSensorData(nd); 
            } 
            SystemState prev = currentState; 
            runStateMachine(); 
            lastState = prev; 
            vTaskDelay(pdMS_TO_TICKS(20)); 
        } 
    }, "stateTask", 8192, NULL, 2, NULL, 1);


    ota_start_task();

    /* == OTA 健康检查：稳定运行 30 秒后确认新固件 == */
    xTaskCreatePinnedToCore([](void*) {
        vTaskDelay(pdMS_TO_TICKS(30000));
        Preferences p;
        p.begin("ota", false);
        if (p.getBool("pending", false)) {
            p.putBool("pending", false);
            p.remove("attempt");
            Serial.println("[OTA] 新固件已验证通过，取消回滚");
        }
        p.end();
        vTaskDelete(NULL);
    }, "otaHealthTask", 2048, NULL, 1, NULL, 1);


    /* == 启动栈监控 == */
    xTaskCreatePinnedToCore([](void*) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        for (;;) {
            Serial.println("\n--- [STACK] 任务栈余量 ---");
            const char *names[] = {"wifiTask","voiceTask","sensorTask",
                                   "stateTask","otaTask","otaHealthTask","stackMon",NULL};
            for (int i = 0; names[i]; i++) {
                TaskHandle_t h = xTaskGetHandle(names[i]);
                if (h == NULL) { Serial.printf("  %s: 未找到\n", names[i]); continue; }
                UBaseType_t fw = uxTaskGetStackHighWaterMark(h);
                Serial.printf("  %-16s  %3u 字 (%u B)\n", names[i], (unsigned)fw, (unsigned)(fw*4));
            }
            Serial.println("---\n");
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    }, "stackMon", 4096, NULL, 1, NULL, 1);

}
