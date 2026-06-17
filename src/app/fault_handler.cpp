#include "app/fault_handler.h"
#include <Arduino.h>
#include <Preferences.h>
#include "esp_system.h"

/*NVS存储分区命名，所有故障记录存在该命名空间下*/
static const char *FAULT_NS = "fault";

/*全局静态故障信息缓存，保存本次上电识别到的故障类型、连续崩溃次数*/
static fault_info_t s_info = { FAULT_REASON_UNKNOWN, 0 };

/*=================代码复位回调=================*/
/*正常软重启前置回调函数*/
static void on_shutdown(void) {
    Preferences p;
    /*打开NVS命名空间，false=读写模式*/
    if (p.begin(FAULT_NS, false)) {
        p.putBool("clean", true);
        p.end();
    }
}
/*故障模块初始化*/
void fault_handler_init(void) {
    esp_register_shutdown_handler(on_shutdown);//注册系统关机回调：主动软重启时触发on_shutdown写入clean正常重启标记
}

/*=================检测上次复位原因=================*/
bool fault_handler_check_boot(void) {
    /*获取芯片底层硬件复位原因*/
    esp_reset_reason_t hw = esp_reset_reason();

    Preferences p;
    p.begin(FAULT_NS, false);

    //读取上次重启标记：true=主动软重启，false=异常死机
    bool clean       = p.getBool("clean", false);
    //读取历史连续崩溃计数
    uint32_t crashes = p.getUInt("crash", 0);
    /* 强制清空干净重启标记，下次异常重启默认无clean标记*/
    p.putBool("clean", false);

    fault_reason_t reason;//存储本次上电识别到的复位类型
    bool is_crash = false; //标记本次是否属于崩溃复位

    /*根据硬件复位源分类转换为自定义故障类型*/
    switch (hw) {
    case ESP_RST_POWERON:
        /*上电开机，清零崩溃计数*/
        reason = FAULT_REASON_POWERON;
        crashes = 0;
        break;
    case ESP_RST_SW:
        /*软件复位：区分手动正常重启/程序异常panic后软复位*/
        reason = clean ? FAULT_REASON_SW_RESET : FAULT_REASON_PANIC;
        if (!clean) is_crash = true;
        break;
    case ESP_RST_PANIC:
        /*内核异常、内存越界、栈溢出等致命异常复位*/
        reason = FAULT_REASON_PANIC;
        is_crash = true;
        break;
    case ESP_RST_TASK_WDT:
        /*任务看门狗超时复位*/
        reason = FAULT_REASON_TASK_WDT;
        is_crash = true;
        break;
    case ESP_RST_INT_WDT:
        /*中断看门狗超时复位*/
        reason = FAULT_REASON_INT_WDT;
        is_crash = true;
        break;
    case ESP_RST_WDT:
        /*RTC硬件看门狗复位*/
        reason = FAULT_REASON_WDT;
        is_crash = true;
        break;
    case ESP_RST_BROWNOUT:
        /*欠压复位，供电电压过低触发保护*/
        reason = FAULT_REASON_BROWNOUT;
        is_crash = true;
        break;
    case ESP_RST_DEEPSLEEP:
        /*深度睡眠唤醒，不属于故障*/
        reason = FAULT_REASON_DEEP_SLEEP;
        break;
    case ESP_RST_SDIO:
    default:
        /* SDIO复位或未识别复位源 */
        reason = FAULT_REASON_UNKNOWN;
        break;
    }

    /*判断本次上电是否是崩溃重启*/
    if (is_crash) {
        crashes++;
    } else {
        crashes = 0;
    }
    p.putUInt("crash", crashes);//将更新后的崩溃次数写入NVS键crash永久保存
    p.end();//关闭NVS存储会话，提交保存

    /*更新全局故障缓存，供外部接口读取*/
    s_info.reason      = reason;
    s_info.crash_count = crashes;

    return is_crash;
}

/*获取故障信息结构体指针*/
const fault_info_t* fault_handler_get_info(void) {
    return &s_info;
}

/*串口打印上次设备复位故障信息*/
void fault_handler_print_info(void) {
    const char *r = fault_reason_to_str(s_info.reason);
    Serial.printf("[FAULT] Last reset: %s", r);
    if (s_info.crash_count > 0) {
        Serial.printf(" (consecutive crashes: %u)", s_info.crash_count);
    }
    Serial.println();
}

/*清空NVS内所有故障记录，重置内存缓存*/
void fault_handler_clear(void) {
    Preferences p;
    p.begin(FAULT_NS, false);
    p.clear();
    p.end();
    s_info.reason      = FAULT_REASON_POWERON;
    s_info.crash_count = 0;
}

/*将自定义故障枚举转为可读字符串，用于日志打印*/
const char* fault_reason_to_str(fault_reason_t r) {
    switch (r) {
    case FAULT_REASON_POWERON:   return "POWERON";              //上电复位
    case FAULT_REASON_PANIC:     return "PANIC/EXCEPTION";      //硬件异常/panic 复位
    case FAULT_REASON_STACK_OVF: return "STACK OVERFLOW";       //FreeRTOS 栈溢出
    case FAULT_REASON_TASK_WDT:  return "TASK WDT";             //任务看门狗超时
    case FAULT_REASON_INT_WDT:   return "INT WDT";              //中断看门狗超时    
    case FAULT_REASON_WDT:       return "OTHER WDT";            //其他看门狗复位
    case FAULT_REASON_BROWNOUT:  return "BROWNOUT";             //掉电检测复位
    case FAULT_REASON_SW_RESET:  return "SW RESTART";           //软件主动重启
    case FAULT_REASON_DEEP_SLEEP:return "DEEP SLEEP WAKE";      //深度睡眠唤醒
    default:                     return "UNKNOWN";              //未知复位
    }
}