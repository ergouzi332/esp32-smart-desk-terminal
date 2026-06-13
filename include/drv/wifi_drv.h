#ifndef DRV_WIFI_DRV_H
#define DRV_WIFI_DRV_H
#include <stdint.h>
extern uint8_t wifi_connected;
void wifi_task_loop(void);
void wifi_connect_start(void);
#endif
