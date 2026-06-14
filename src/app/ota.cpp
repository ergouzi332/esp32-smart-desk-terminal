#include "app/ota.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <nvs_flash.h>
#include <nvs.h>

static String read_local_version() {
    nvs_handle_t h;
    char buf[16];
    size_t len = sizeof(buf);
    if (nvs_open("ota", NVS_READONLY, &h) == ESP_OK) {
        esp_err_t e = nvs_get_str(h, "version", buf, &len);
        nvs_close(h);
        if (e == ESP_OK) return String(buf);
    }
    return String(OTA_DEFAULT_VERSION);
}

static void save_local_version(const String &v) {
    nvs_handle_t h;
    if (nvs_open("ota", NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, "version", v.c_str());
        nvs_commit(h);
        nvs_close(h);
    }
}

static void ota_task(void* param) {
    for (;;) {
        int r = 0;
        while (WiFi.status() != WL_CONNECTED && r < 100) {
            vTaskDelay(pdMS_TO_TICKS(100)); r++;
        }
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[OTA] WiFi 未连接");
            vTaskDelay(pdMS_TO_TICKS(30000)); continue;
        }

        String url = "http://" + String(OTA_SERVER_IP) + ":" + String(OTA_SERVER_PORT) + "/";
        Serial.print("[OTA] 检查更新："); Serial.println(url + "version.txt");

        HTTPClient http;
        http.begin(url + "version.txt");
        http.setTimeout(5000);
        int code = http.GET();

        if (code == 200) {
            String server_ver = http.getString(); server_ver.trim(); http.end();
            String local_ver = read_local_version();

            Serial.print("[OTA] 本地："); Serial.print(local_ver);
            Serial.print(" -> 服务器："); Serial.println(server_ver);

            if (local_ver == server_ver) {
                Serial.println("[OTA] 已是最新");
            } else if (server_ver.length() == 0) {
                Serial.println("[OTA] 服务器返回空");
            } else {
                Serial.println("[OTA] 发现新版本！开始下载...");
                http.begin(url + "firmware.bin");
                http.setTimeout(10000);
                int fwCode = http.GET();
                if (fwCode == 200) {
                    int total = http.getSize();
                    if (total > 0 && Update.begin(total)) {
                        size_t w = Update.writeStream(http.getStream());
                        if (w == total && Update.end()) {
                            save_local_version(server_ver);
                            Serial.println("[OTA] 升级成功！重启中...");
                            delay(100); esp_restart();
                        } else {
                            Serial.print("[OTA] 失败："); Serial.println(Update.errorString());
                        }
                    }
                }
            }
        } else {
            Serial.print("[OTA] 服务器连接失败："); Serial.println(code);
        }
        http.end();
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void ota_start_task(void) {
    xTaskCreatePinnedToCore(ota_task, "otaTask", 8192, NULL, 1, NULL, 1);
}
