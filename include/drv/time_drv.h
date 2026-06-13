#ifndef DRV_TIME_DRV_H
#define DRV_TIME_DRV_H

/*获取当前时间*/
void getTime();
void updateTimeFromRTC(void);

/*时间数据结构体*/
typedef struct 
{ 
    int year; 
    int month; 
    int day; 
    int hour; 
    int minute; 
} Time_Typedef;

extern Time_Typedef Time_State;  // 系统时间状态变量

#endif