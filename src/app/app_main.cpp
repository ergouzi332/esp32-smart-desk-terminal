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

/*OTA新固件最大允许启动失败次数，达到该值自动回滚旧分区*/
#define OTA_MAX_BOOT_ATTEMPTS 4

void App_Init(void)
{
    /*====修正OTA引导分区：确保USB烧录后从当前运行分区启动====*/
    {
        //获取当前正在运行的OTA分区
        const esp_partition_t *running = esp_ota_get_running_partition();
        //获取芯片配置的开机引导分区
        const esp_partition_t *boot = esp_ota_get_boot_partition();
        //若引导分区和实际运行分区不一致，修复引导标记
        if (running != NULL && boot != NULL && running != boot) {
            Serial.printf("[BOOT] 引导分区(%s)与运行分区(%s) 不匹配，正在修正\n",
                          boot->label, running->label);
            //修改开机引导分区为当前运行分区
            esp_ota_set_boot_partition(running);
            Serial.println("[BOOT] 重启后生效");
        }
    }

    /*====故障检测：上电最先执行，识别上一次设备复位的根本原因====*/
    fault_handler_init();                   //初始化故障模块，注册正常软关机回调
    if (fault_handler_check_boot()) {       //读取NVS+硬件复位源，判断上次是否异常崩溃
        fault_handler_print_info();         //串口打印故障/复位日志，用于定位崩溃根源
    }

    /*====OTA回滚检测：仅pending标记为true时生效，统计新固件连续启动尝试次数====*/
    {
        Preferences pref;
        pref.begin("ota", false); //打开ota专属NVS命名空间，读写模式
        bool pending = pref.getBool("pending", false); //pending=true代表当前是刚升级未验证的新固件
        if (pending) {
            //读取历史启动次数并自增1
            uint8_t attempt = pref.getUChar("attempt", 0) + 1;
            pref.putUChar("attempt", attempt); //更新新的启动次数存入NVS
            pref.end();
            Serial.printf("[OTA] 启动尝试 %d/%d\n", attempt, OTA_MAX_BOOT_ATTEMPTS);
            //连续启动失败达到阈值，执行回滚逻辑
            if (attempt >= OTA_MAX_BOOT_ATTEMPTS) {
                // 获取备用旧OTA分区
                const esp_partition_t *part = esp_ota_get_next_update_partition(NULL);
                if (part) {
                    //设置开机引导分区为旧固件分区
                    esp_ota_set_boot_partition(part);
                    Serial.println("[OTA] 启动失败次数过多，回滚到旧分区");
                    delay(100); //等待分区配置写入Flash完成
                    esp_restart(); //重启设备，运行旧稳定固件
                }
            }
        } else {
            //无待验证新固件，直接关闭NVS会话
            pref.end();
        }
    }

    // 创建共享数据互斥锁，多任务访问全局传感器/状态数据时防冲突
    sharedDataMutex = xSemaphoreCreateMutex();
    // 互斥锁创建失败，死锁停机
    if (sharedDataMutex == NULL) { while (1) { } }
    // 语音指令消息队列，缓存SU-03T串口指令
    voiceQueue = xQueueCreate(8, sizeof(uint8_t));
    // 传感器数据队列，仅保存最新一组温湿度电量数据
    sensorQueue = xQueueCreate(1, sizeof(SensorData_t));

    /*WiFi状态检测任务：绑定核心1，500ms周期轮询WiFi状态 */
    xTaskCreatePinnedToCore([](void* p) 
    { for (;;) 
        { wifi_task_loop(); // WiFi业务逻辑循环
            vTaskDelay(pdMS_TO_TICKS(500)); 
        } 
    }, 
    "wifiTask", 4096, NULL, 1, NULL, 1);

    /* 语音监听任务：绑定核心0，10ms读取串口SU-03T语音指令存入队列 */
    xTaskCreatePinnedToCore([](void* p) 
    { for (;;) 
        { if (Serial.available()) 
            { uint8_t c = Serial.read(); 
                xQueueSend(voiceQueue, &c, 0); // 非阻塞发送指令到队列
            } 
            vTaskDelay(pdMS_TO_TICKS(10)); 
        } 
    }, 
    "voiceTask", 4096, NULL, 3, NULL, 0);

    /* 传感器采集任务：绑定核心0，2秒读取一次温湿度、电池电量并更新队列 */
    xTaskCreatePinnedToCore([](void* p) 
    { SensorData_t sd = {0,0,0,false}; 
    getTime(); // 初始化系统时间
    for (;;) 
    { sd.temperature = dht11.readTemperature(); // 读取DHT11温度
      sd.humidity = dht11.readHumidity();         // 读取DHT11湿度
      GetCur_Power();                             // 获取当前电池电量百分比
      sd.batteryPercent = CurBattery;
      sd.valid = true; // 标记数据有效
      xQueueOverwrite(sensorQueue, &sd); // 覆盖写入最新传感器数据
      vTaskDelay(pdMS_TO_TICKS(2000)); 
    } 
    }, 
    "sensorTask", 4096, NULL, 2, NULL, 0);

    /* 核心状态机任务：绑定核心1，20ms循环，处理语音指令、刷新传感器数据、执行业务逻辑 */
    xTaskCreatePinnedToCore([](void* p) 
    { uint8_t c; 
        SensorData_t nd; 
        for (;;) 
        { 
            // 读取语音队列指令，切换设备显示状态
            if (xQueueReceive(voiceQueue, &c, 0) == pdPASS) 
            { 
                if (c==0xAA) currentState=STATE_SHOW_TIME;
                else if (c==0xBB) currentState=STATE_SHOW_WEATHER;
                else if (c==0xCC) currentState=STATE_SHOW_HR;
                else if (c==0xDD) currentState=STATE_SHOW_ENV;
                else if (c==0xEE) currentState=STATE_SHOW_BATTERY;
            } 
            // 获取最新传感器数据，更新全局缓存
            if (xQueueReceive(sensorQueue, &nd, 0) == pdPASS) 
            { 
                setLatestSensorData(nd); 
            } 
            // 保存切换前状态，执行一轮状态机逻辑
            SystemState prev = currentState; 
            runStateMachine(); 
            lastState = prev; 
            vTaskDelay(pdMS_TO_TICKS(20)); 
        } 
    }, "stateTask", 8192, NULL, 2, NULL, 1);

    // 启动后台OTA升级监听任务，处理远程固件下载
    ota_start_task();

    /* == OTA健康校验任务：延迟30秒运行，新固件稳定无崩溃则清除pending标记，永久关闭回滚保护 == */
    xTaskCreatePinnedToCore([](void*) {
        // 等待30秒，30秒内不崩溃即判定固件可用
        vTaskDelay(pdMS_TO_TICKS(30000));
        Preferences p;
        p.begin("ota", false);
        // 仍为待验证状态，清除标记、删除启动计数字段
        if (p.getBool("pending", false)) {
            p.putBool("pending", false);
            p.remove("attempt");
            Serial.println("[OTA] 新固件已验证通过，取消回滚");
        }
        p.end();
        vTaskDelete(NULL); // 校验完成销毁自身任务
    }, "otaHealthTask", 2048, NULL, 1, NULL, 1);

    /* == 栈监控任务：5秒后启动，每10秒打印全部任务栈剩余高水位，用于排查栈溢出风险 == */
    xTaskCreatePinnedToCore([](void*) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        for (;;) {
            Serial.println("\n--- [STACK] 任务栈余量 ---");
            // 全部业务任务名称数组
            const char *names[] = {"wifiTask","voiceTask","sensorTask",
                                   "stateTask","otaTask","otaHealthTask","stackMon",NULL};
            for (int i = 0; names[i]; i++) {
                TaskHandle_t h = xTaskGetHandle(names[i]); // 通过名称获取任务句柄
                if (h == NULL) { Serial.printf("  %s: 未找到\n", names[i]); continue; }
                //获取任务最小剩余栈深度（高水位标记）
                UBaseType_t fw = uxTaskGetStackHighWaterMark(h);
                Serial.printf("  %-16s  %3u 字 (%u B)\n", names[i], (unsigned)fw, (unsigned)(fw*4));
            }
            Serial.println("---\n");
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    }, "stackMon", 4096, NULL, 1, NULL, 1);

}