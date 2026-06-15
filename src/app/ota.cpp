#include "app/ota.h"
#include "app/ota_sha256.h"
#include "app/ota_aes.h"
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

/* Flash 写回调 */
static size_t g_ota_written = 0;
static void ota_write_cb(uint8_t *data, size_t len) {
    if (len > 0) {
        Update.write(data, len);
        g_ota_written += len;
    }
}

/* SHA256 哈希回调 */
static ota_sha256_ctx g_sha_ctx;
static void ota_sha_cb(const uint8_t *data, size_t len) {
    if (len > 0) {
        ota_sha256_update(&g_sha_ctx, data, len);
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

        String base = "http://" + String(OTA_SERVER_IP) + ":" + String(OTA_SERVER_PORT) + "/";
        Serial.print("[OTA] 检查更新："); Serial.println(base + "version.txt");

        HTTPClient http;

        /* 1. 获取版本号 */
        http.begin(base + "version.txt");
        http.setTimeout(5000);
        int code = http.GET();
        if (code != 200) {
            Serial.print("[OTA] 服务器连接失败："); Serial.println(code);
            http.end();
            vTaskDelay(pdMS_TO_TICKS(10000)); continue;
        }
        String server_ver = http.getString(); server_ver.trim(); http.end();

        String local_ver = read_local_version();
        Serial.print("[OTA] 本地："); Serial.print(local_ver);
        Serial.print(" -> 服务器："); Serial.println(server_ver);

        if (local_ver == server_ver) {
            Serial.println("[OTA] 已是最新版本，无需升级");
        } else if (server_ver.length() == 0) {
            Serial.println("[OTA] 服务器版本为空");
        } else {
            Serial.println("[OTA] 发现新版本！开始下载...");
 
            /* 清除旧的回滚标记：避免本次升级失败后残留旧 pending 导致误回滚 */
            {
                Preferences p;
                p.begin("ota", false);
                p.remove("pending");
                p.remove("attempt");
                p.end();
            }

            /* 2. 获取 SHA256 校验值 */
            http.begin(base + "firmware.sha256");
            http.setTimeout(5000);
            String expected_hash;
            if (http.GET() == 200) {
                expected_hash = http.getString(); expected_hash.trim(); expected_hash.toLowerCase();
            }
            http.end();

            /* 3. 下载加密固件 firmware.enc */
            http.begin(base + "firmware.enc");
            http.setTimeout(10000);
            int fwCode = http.GET();
            if (fwCode != 200) {
                Serial.print("[OTA] 固件下载失败："); Serial.println(fwCode);
                http.end();
                vTaskDelay(pdMS_TO_TICKS(10000)); continue;
            }

            int total = http.getSize();
            if (total <= 0 || total % 16 != 0) {
                Serial.println("[OTA] 加密固件大小无效");
                http.end();
                vTaskDelay(pdMS_TO_TICKS(10000)); continue;
            }

            if (!Update.begin((size_t)total)) {
                Serial.print("[OTA] 准备更新失败："); Serial.println(Update.errorString());
                http.end();
                vTaskDelay(pdMS_TO_TICKS(10000)); continue;
            }

            /* 4. 初始化 AES 流式解密和 SHA256 */
            ota_aes_stream_ctx_t aes_ctx;
            const uint8_t *key = ota_aes_get_key();
            const uint8_t *iv  = ota_aes_get_iv();
            ota_aes_stream_init(&aes_ctx, key, iv);

            ota_sha256_init(&g_sha_ctx);
            g_ota_written = 0;

            /* 5. 流式读取、解密 */
            uint8_t buf[128];
            WiFiClient *s = http.getStreamPtr();
            size_t total_read = 0;

            while (http.connected() && total_read < (size_t)total) {
                size_t avail = s->available();
                if (avail == 0) { vTaskDelay(1); continue; }
                if (avail > sizeof(buf)) avail = sizeof(buf);
                int n = s->readBytes(buf, avail);
                if (n <= 0) continue;

                size_t aligned = ((size_t)n / 16) * 16;
                if (aligned > 0) {
                    if (ota_aes_stream_feed(&aes_ctx, buf, aligned,
                                            ota_write_cb, ota_sha_cb) != 0) {
                        Serial.println("[OTA] AES 解密错误");
                        Update.abort();
                        break;
                    }
                    total_read += aligned;
                }
                if (aligned < (size_t)n) {
                    total_read += (n - aligned);
                }
            }

            if (total_read == (size_t)total) {
                /* 6. 处理最后一块 PKCS7 填充 */
                size_t last_len;
                if (ota_aes_stream_finish(&aes_ctx, &last_len,
                                          ota_write_cb, ota_sha_cb) != 0) {
                    Serial.println("[OTA] AES 收尾错误");
                    Update.abort();
                } else {
                    /* 7. 计算 SHA256 并校验 */
                    uint8_t hash[32];
                    ota_sha256_final(&g_sha_ctx, hash);
                    String hex_hash;
                    for (int i = 0; i < 32; i++) {
                        if (hash[i] < 16) hex_hash += "0";
                        hex_hash += String(hash[i], HEX);
                    }
                    hex_hash.toLowerCase();
                    Serial.print("[OTA] SHA256："); Serial.println(hex_hash);

                    if (expected_hash.length() == 0 || hex_hash == expected_hash) {
                        if (Update.end()) {
                            /* 设置 OTA 回滚验证标志：下次启动先进入验证期 */
                            {
                                Preferences p;
                                p.begin("ota", false);
                                p.putBool("pending", true);
                                p.putUChar("attempt", 0);
                                p.end();
                            }
                            save_local_version(server_ver);
                            Serial.println("[OTA] 升级成功！重启中...");
                            delay(100);
                            esp_restart();
                        } else {
                            Serial.print("[OTA] 更新失败："); Serial.println(Update.errorString());
                        }
                    } else {
                        Serial.println("[OTA] SHA256 不匹配，取消升级");
                        Update.abort();
                    }
                }
            }
            http.end();
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void ota_start_task(void) {
    xTaskCreatePinnedToCore(ota_task, "otaTask", 8192, NULL, 1, NULL, 1);
}
