#include "app/fault_handler.h"
#include <Arduino.h>
#include <Preferences.h>
#include "esp_system.h"

static const char *FAULT_NS = "fault";
static fault_info_t s_info = { FAULT_REASON_UNKNOWN, 0 };

/* Called before esp_restart() to mark intentional restart */
static void on_shutdown(void) {
    Preferences p;
    if (p.begin(FAULT_NS, false)) {
        p.putBool("clean", true);
        p.end();
    }
}

void fault_handler_init(void) {
    esp_register_shutdown_handler(on_shutdown);
}

bool fault_handler_check_boot(void) {
    esp_reset_reason_t hw = esp_reset_reason();

    Preferences p;
    p.begin(FAULT_NS, false);

    bool clean       = p.getBool("clean", false);
    uint32_t crashes = p.getUInt("crash", 0);
    /* Reset clean flag: put instead of remove to avoid NOT_FOUND warning */
    p.putBool("clean", false);

    fault_reason_t reason;
    bool is_crash = false;

    switch (hw) {
    case ESP_RST_POWERON:
        reason = FAULT_REASON_POWERON;
        crashes = 0;
        break;
    case ESP_RST_SW:
        reason = clean ? FAULT_REASON_SW_RESET : FAULT_REASON_PANIC;
        if (!clean) is_crash = true;
        break;
    case ESP_RST_PANIC:
        reason = FAULT_REASON_PANIC;
        is_crash = true;
        break;
    case ESP_RST_TASK_WDT:
        reason = FAULT_REASON_TASK_WDT;
        is_crash = true;
        break;
    case ESP_RST_INT_WDT:
        reason = FAULT_REASON_INT_WDT;
        is_crash = true;
        break;
    case ESP_RST_WDT:
        reason = FAULT_REASON_WDT;
        is_crash = true;
        break;
    case ESP_RST_BROWNOUT:
        reason = FAULT_REASON_BROWNOUT;
        is_crash = true;
        break;
    case ESP_RST_DEEPSLEEP:
        reason = FAULT_REASON_DEEP_SLEEP;
        break;
    case ESP_RST_SDIO:
    default:
        reason = FAULT_REASON_UNKNOWN;
        break;
    }

    /* Manage crash count:
     *   Crash -> increment and save
     *   Clean boot -> always reset to 0 and save
     *   (Previously the reset path only saved when > 0, leaving stale value in NVS) */
    if (is_crash) {
        crashes++;
    } else {
        crashes = 0;
    }
    p.putUInt("crash", crashes);
    p.end();

    s_info.reason      = reason;
    s_info.crash_count = crashes;

    return is_crash;
}

const fault_info_t* fault_handler_get_info(void) {
    return &s_info;
}

void fault_handler_print_info(void) {
    const char *r = fault_reason_to_str(s_info.reason);
    Serial.printf("[FAULT] Last reset: %s", r);
    if (s_info.crash_count > 0) {
        Serial.printf(" (consecutive crashes: %u)", s_info.crash_count);
    }
    Serial.println();
}

void fault_handler_clear(void) {
    Preferences p;
    p.begin(FAULT_NS, false);
    p.clear();
    p.end();
    s_info.reason      = FAULT_REASON_POWERON;
    s_info.crash_count = 0;
}

const char* fault_reason_to_str(fault_reason_t r) {
    switch (r) {
    case FAULT_REASON_POWERON:   return "POWERON";
    case FAULT_REASON_PANIC:     return "PANIC/EXCEPTION";
    case FAULT_REASON_STACK_OVF: return "STACK OVERFLOW";
    case FAULT_REASON_TASK_WDT:  return "TASK WDT";
    case FAULT_REASON_INT_WDT:   return "INT WDT";
    case FAULT_REASON_WDT:       return "OTHER WDT";
    case FAULT_REASON_BROWNOUT:  return "BROWNOUT";
    case FAULT_REASON_SW_RESET:  return "SW RESTART";
    case FAULT_REASON_DEEP_SLEEP:return "DEEP SLEEP WAKE";
    default:                     return "UNKNOWN";
    }
}
