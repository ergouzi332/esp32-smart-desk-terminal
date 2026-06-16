#include "app/stack_monitor.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* Known task names, matching xTaskCreate calls in app_main.cpp */
static const char *s_known_tasks[] = {
    "wifiTask",
    "voiceTask",
    "sensorTask",
    "stateTask",
    "otaTask",
    "otaHealthTask",
    "stackMon",
    NULL
};

/* ------------------------------------------------------------------ */
/*  FreeRTOS stack overflow hook                                      */
/*  Called by kernel when configCHECK_FOR_STACK_OVERFLOW==2           */
/* ------------------------------------------------------------------ */
extern "C" void vApplicationStackOverflowHook(void *pxTask, char *pcTaskName) {
    (void)pxTask;
    Serial.printf("\n*** FATAL: Task '%s' stack overflow! ***\n",
                  pcTaskName ? pcTaskName : "?");
}

/* ------------------------------------------------------------------ */
/*  Periodic monitor: prints stack high-water marks every 10 seconds  */
/* ------------------------------------------------------------------ */
static void stack_monitor_task(void *param) {
    (void)param;
    vTaskDelay(pdMS_TO_TICKS(5000));

    for (;;) {
        Serial.println("\n--- [STACK] Free stack (words) ---");
        for (int i = 0; s_known_tasks[i] != NULL; i++) {
            TaskHandle_t h = xTaskGetHandle(s_known_tasks[i]);
            if (h == NULL) {
                Serial.printf("  %s: NOT FOUND\n", s_known_tasks[i]);
                continue;
            }

            UBaseType_t free_words = uxTaskGetStackHighWaterMark(h);
            uint32_t free_bytes = free_words * sizeof(StackType_t);

            Serial.printf("  %-16s  %3u words  (%u B)",
                          s_known_tasks[i],
                          (unsigned)free_words,
                          (unsigned)free_bytes);

            if (free_words < 12) {
                Serial.print("  *** LOW ***");
            }
            Serial.println();
        }
        Serial.println("---\n");

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/* ------------------------------------------------------------------ */
/*  Public API                                                        */
/* ------------------------------------------------------------------ */

void stack_monitor_start(void) {
    xTaskCreatePinnedToCore(
        stack_monitor_task,
        "stackMon",
        4096,
        NULL,
        1,
        NULL,
        1
    );
}

uint32_t stack_monitor_get_free(const char *task_name) {
    TaskHandle_t h = xTaskGetHandle(task_name);
    if (h == NULL) return 0;
    return (uint32_t)uxTaskGetStackHighWaterMark(h) * 4;
}

void stack_monitor_print_all(void) {
    Serial.println("[STACK] Manual query:");
    for (int i = 0; s_known_tasks[i] != NULL; i++) {
        TaskHandle_t h = xTaskGetHandle(s_known_tasks[i]);
        if (!h) {
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
