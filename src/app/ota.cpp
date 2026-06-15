#include "app/ota.h"
#include "app/ota_sha256.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Preferences.h>

static String read_local_version() {
    Preferences p;
    p.begin("ota", true);
    String v = p.getString("version", OTA_DEFAULT_VERSION);
    p.end();
    return v;
}

static void save_local_version(const String &v) {
    Preferences p;
    p.begin("ota", false);
    p.putString("version", v);
    p.end();
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
                Serial.println("[OTA] 已是最新版本，无需升级");
            } else if (server_ver.length() == 0) {
                Serial.println("[OTA] 服务器返回空版本号");
            } else {
                Serial.println("[OTA] 发现新版本！开始下载...");

                http.begin(url + "firmware.sha256");
                http.setTimeout(5000);
                int shaCode = http.GET();
                String expected_hash;
                if (shaCode == 200) {
                    expected_hash = http.getString(); expected_hash.trim(); expected_hash.toLowerCase();
                }
                http.end();

                http.begin(url + "firmware.bin");
                http.setTimeout(10000);
                int fwCode = http.GET();
                if (fwCode == 200) {
                    int total = http.getSize();
                    if (total > 0 && Update.begin(total)) {
                        ota_sha256_ctx sha;
                        ota_sha256_init(&sha);
                        uint8_t buf[128];
                        WiFiClient *s = http.getStreamPtr();
                        size_t written = 0;
                        while (http.connected() && written < (size_t)total) {
                            size_t avail = s->available();
                            if (avail > 0) {
                                if (avail > sizeof(buf)) avail = sizeof(buf);
                                int r2 = s->readBytes(buf, avail);
                                if (r2 > 0) {
                                    Update.write(buf, r2);
                                    ota_sha256_update(&sha, buf, r2);
                                    written += r2;
                                }
                            }
                        }
                        uint8_t hash[32];
                        ota_sha256_final(&sha, hash);
                        String hex_hash;
                        for (int i = 0; i < 32; i++) {
                            if (hash[i] < 16) hex_hash += "0";
                            hex_hash += String(hash[i], HEX);
                        }
                        hex_hash.toLowerCase();
                        Serial.print("[OTA] SHA256: "); Serial.println(hex_hash);

                        if (expected_hash.length() == 0 || hex_hash == expected_hash) {
                            if (Update.end()) {
                                save_local_version(server_ver);
                                Serial.println("[OTA] 升级成功！重启中...");
                                delay(100); esp_restart();
                            } else {
                                Serial.print("[OTA] 失败："); Serial.println(Update.errorString());
                            }
                        } else {
                            Serial.println("[OTA] SHA256 不匹配！取消升级");
                            Update.abort();
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
