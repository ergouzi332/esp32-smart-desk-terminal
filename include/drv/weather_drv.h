#ifndef DRV_WEATHER_DRV_H
#define DRV_WEATHER_DRV_H

/*天气信息结构体*/
typedef struct {
    char cityName[10];
    char weatherText[10];
    int temperature;
} Weather_Typedef;

extern Weather_Typedef Weather_State;  // 天气数据状态变量

/* 获取网络天气数据*/
void getWeather();

#endif