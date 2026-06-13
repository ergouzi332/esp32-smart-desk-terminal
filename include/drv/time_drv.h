#ifndef DRV_TIME_DRV_H
#define DRV_TIME_DRV_H
void getTime();
typedef struct { int year; int month; int day; int hour; int minute; } Time_Typedef;
extern Time_Typedef Time_State;
#endif
