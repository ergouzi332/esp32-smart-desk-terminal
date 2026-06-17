#include "app/stack_monitor.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/*系统全部业务任务名列表*/
static const char *s_known_tasks[] = {
    "wifiTask",
    "voiceTask",
    "sensorTask",
    "stateTask",
    "otaTask",
    "otaHealthTask",
    "stackMon",
    NULL   //数组结束标记，用于循环遍历判断终止
};

/*FreeRTOS内核栈溢出钩子函数*/
extern "C" void vApplicationStackOverflowHook(void *pxTask, char *pcTaskName) {
    //未使用参数，消除编译警告
    (void)pxTask;
    /*串口打印致命错误，输出发生栈溢出的任务名*/
    Serial.printf("\n*** FATAL: Task '%s' stack overflow! ***\n",
                  pcTaskName ? pcTaskName : "?");
}

/*后台栈监控循环任务函数*/
static void stack_monitor_task(void *param) {
    (void)param;
    /*上电延时5秒再开始监控，避免开机日志刷屏*/
    vTaskDelay(pdMS_TO_TICKS(5000));

    for (;;)
    {
        Serial.println("\n--- [STACK] Free stack (words) ---");
        /*遍历所有预定义业务任务*/
        for (int i = 0; s_known_tasks[i] != NULL; i++)
        {
            /*通过任务名获取任务句柄*/
            TaskHandle_t h = xTaskGetHandle(s_known_tasks[i]);
            /*句柄为空代表任务未创建/已销毁*/
            if (h == NULL)
            {
                Serial.printf("  %s: NOT FOUND\n", s_known_tasks[i]);
                continue;
            }
            /*获取任务栈高水位标记：运行期间剩余最小栈空间(单位word)*/
            UBaseType_t free_words = uxTaskGetStackHighWaterMark(h);
            /*转换为字节数，ESP32 StackType_t占4字节(1words = 4bytes)*/
            uint32_t free_bytes = free_words * sizeof(StackType_t);

            // 格式化打印任务名、剩余栈字数、剩余字节数
            Serial.printf("  %-16s  %3u words  (%u B)",
                          s_known_tasks[i],
                          (unsigned)free_words,
                          (unsigned)free_bytes);

            //自定义预警阈值：剩余栈小于12字标记低栈警告
            if (free_words < 12)
            {
                Serial.print("  *** LOW ***");
            }
            Serial.println();
        }
        Serial.println("---\n");

        /*间隔30秒执行一次栈巡检*/
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

/*启动栈监控后台任务*/
void stack_monitor_start(void) {
    xTaskCreatePinnedToCore(
        stack_monitor_task,    // 任务入口函数
        "stackMon",            // 任务名称
        4096,                  // 分配任务栈大小
        NULL,                  // 传入参数
        1,                     // 任务优先级
        NULL,                  // 任务句柄不保存
        1                      // 绑定至CPU核心1
    );
}

/*查询指定任务当前剩余栈字节数*/
uint32_t stack_monitor_get_free(const char *task_name) {
    TaskHandle_t h = xTaskGetHandle(task_name);
    if (h == NULL) return 0;
    return (uint32_t)uxTaskGetStackHighWaterMark(h) * 4;
}

/*手动打印全部业务任务栈余量信息，用于调试手动触发查看 */
void stack_monitor_print_all(void) {
    Serial.println("[STACK] Manual query:");
    for (int i = 0; s_known_tasks[i] != NULL; i++)
    {
        TaskHandle_t h = xTaskGetHandle(s_known_tasks[i]);
        if (!h)
        {
            Serial.printf("  %s: NOT FOUND\n", s_known_tasks[i]);
            continue;
        }

        UBaseType_t free_w = uxTaskGetStackHighWaterMark(h);
        uint32_t free_b = free_w * sizeof(StackType_t);

        Serial.printf("  %s: %u words (%u B)\n",
                      s_known_tasks[i],
                      (unsigned)free_w,
                      (unsigned)free_b);
    }
}