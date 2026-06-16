#ifndef APP_FAULT_HANDLER_H
#define APP_FAULT_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 复位原因枚举，与 esp_reset_reason_t 对应，方便 OLED 显示 */
typedef enum {
    FAULT_REASON_POWERON   = 0,  /* 上电复位 */
    FAULT_REASON_PANIC     = 1,  /* 硬件异常/panic 复位 */
    FAULT_REASON_STACK_OVF = 2,  /* FreeRTOS 栈溢出 */
    FAULT_REASON_TASK_WDT  = 3,  /* 任务看门狗超时 */
    FAULT_REASON_INT_WDT   = 4,  /* 中断看门狗超时 */
    FAULT_REASON_WDT       = 5,  /* 其他看门狗 */
    FAULT_REASON_BROWNOUT  = 6,  /* 掉电检测 */
    FAULT_REASON_SW_RESET  = 7,  /* 软件主动重启 */
    FAULT_REASON_DEEP_SLEEP= 8,  /* 深度睡眠唤醒 */
    FAULT_REASON_UNKNOWN   = 9,  /* 未知 */
} fault_reason_t;

/* 上次崩溃的快照信息 */
typedef struct {
    fault_reason_t reason;       /* 复位原因 */
    uint32_t       crash_count;  /* 连续崩溃次数（>1 表示崩循环） */
} fault_info_t;

/*
 * 初始化故障处理系统：
 * - 注册 esp_restart 前的关闭回调
 */
void fault_handler_init(void);

/*
 * 开机后调用，检测上次复位是否为异常。
 * 如果返回 true，可通过 fault_handler_get_info() 获取详情。
 * 内部会管理连续崩溃计数，并在 OLED 就绪后尝试显示。
 */
bool fault_handler_check_boot(void);

/* 获取上一步检测到的故障信息 */
const fault_info_t* fault_handler_get_info(void);

/* 将故障信息打印到 Serial */
void fault_handler_print_info(void);

/* 清除 NVS 中的崩溃标记 */
void fault_handler_clear(void);

/* 将复位原因转换为可读字符串 */
const char* fault_reason_to_str(fault_reason_t r);

#ifdef __cplusplus
}
#endif

#endif /* APP_FAULT_HANDLER_H */
