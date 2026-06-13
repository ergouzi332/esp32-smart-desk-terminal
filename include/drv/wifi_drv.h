#ifndef DRV_WIFI_DRV_H
#define DRV_WIFI_DRV_H

#include <stdint.h>

extern uint8_t wifi_connected;   //WiFi连接状态标志，非0表示已连接

/*WiFi任务循环函数*/
void wifi_task_loop(void);

/*启动WiFi连接流程*/
void wifi_connect_start(void);

#endif