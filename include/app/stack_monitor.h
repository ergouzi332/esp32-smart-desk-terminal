#ifndef APP_STACK_MONITOR_H
#define APP_STACK_MONITOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*启动后台栈监控任务*/
void stack_monitor_start(void);

/*======手动调试函数========*/
/*根据任务名查询该任务最小剩余栈空间*/
uint32_t stack_monitor_get_free(const char *task_name);
/*立即串口打印所有预设业务任务的栈使用信息*/
void stack_monitor_print_all(void);

#ifdef __cplusplus
}
#endif

#endif