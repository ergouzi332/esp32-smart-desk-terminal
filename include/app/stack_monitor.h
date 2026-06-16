#ifndef APP_STACK_MONITOR_H
#define APP_STACK_MONITOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 启动栈监控任务（每 10 秒检查一次所有任务的栈高水位） */
void stack_monitor_start(void);

/* 获取指定名称任务的剩余栈空间（字节），返回 0 表示任务不存在 */
uint32_t stack_monitor_get_free(const char *task_name);

/* 打印所有任务栈使用情况到 Serial */
void stack_monitor_print_all(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_STACK_MONITOR_H */
