#include "app/ota.h"
#include "app/ota_sha256.h"
#include "app/ota_aes.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Preferences.h>

/*读取本地存储的固件版本号*/
static String read_local_version() {
    Preferences p;
    p.begin("ota", true);
    String v = p.getString("version", OTA_DEFAULT_VERSION);//若键不存在/读取失败，返回兜底默认版本OTA_DEFAULT_VERSION
    p.end();
    return v;
}
/*将新版本号写入NVS持久化存储*/
static void save_local_version(const String &v) {
    Preferences p;
    p.begin("ota", false);
    p.putString("version", v);
    p.end();
}
/*Flash分片写回调*/
static size_t g_ota_written = 0;//全局静态变量：记录当前OTA已写入Flash的总字节数
static void ota_write_cb(uint8_t *data, size_t len) {
    if (len > 0) {
        Update.write(data, len);//将分片固件数据写入Flash完成OTA烧录
        g_ota_written += len;
    }
}
/*SHA256 哈希回调*/
static ota_sha256_ctx g_sha_ctx;//全局静态SHA256上下文结构体，保存哈希运算中间状态
static void ota_sha_cb(const uint8_t *data, size_t len) {
    if (len > 0) {
        ota_sha256_update(&g_sha_ctx, data, len);//分段更新SHA256上下文，累加计算哈希值
    }
}
/*OTA升级任务*/
static void ota_task(void* param) {
    for (;;) {
        int r = 0;
        /*循环等待WiFi连接，最大重试100次，每次间隔100ms*/
        while (WiFi.status() != WL_CONNECTED && r < 100) {
            vTaskDelay(pdMS_TO_TICKS(100)); r++;
        }
        /*超时仍未连接WiFi，打印提示并延时30秒后进入下一轮循环*/
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[OTA] WiFi 未连接");
            vTaskDelay(pdMS_TO_TICKS(30000)); continue;
        }
        /*拼接出OTA服务器的版本检测地址*/
        String base = "http://" + String(OTA_SERVER_IP) + ":" + String(OTA_SERVER_PORT) + "/";
        Serial.print("[OTA] 检查更新："); Serial.println(base + "version.txt");

        HTTPClient http;

        /*================获取版本号==================*/
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
        /*================版本不同，进入OTA==================*/
        if (local_ver == server_ver) {
            Serial.println("[OTA] 已是最新版本，无需升级");
        } else if (server_ver.length() == 0) {
            Serial.println("[OTA] 服务器版本为空");
        } else {
            Serial.println("[OTA] 发现新版本！开始下载...");
 
            /*==============OTA：获取服务端的SHA256校验值=================*/
            http.begin(base + "firmware.sha256");
            http.setTimeout(5000);
            String expected_hash;
            if (http.GET() == 200) {
                expected_hash = http.getString(); expected_hash.trim(); expected_hash.toLowerCase();
            }
            http.end();
            /*==============OTA：下载服务器端加密固件firmware.enc=================*/
            http.begin(base + "firmware.enc");
            http.setTimeout(10000);
            int fwCode = http.GET();
            if (fwCode != 200) {
                Serial.print("[OTA] 固件下载失败："); Serial.println(fwCode);
                http.end();
                vTaskDelay(pdMS_TO_TICKS(10000)); continue;
            }
            //AES分组为16字节，加密后密文长度一定是16的整数倍；
            //如果长度≤0（获取失败）或不能被16整除，判定文件损坏、非法加密包
            int total = http.getSize();
            if (total <= 0 || total % 16 != 0) {
                Serial.println("[OTA] 加密固件大小无效");
                http.end();
                vTaskDelay(pdMS_TO_TICKS(10000)); continue;
            }
            /*===============初始化ESP32 OTA分区，分配对应大小的Flash空间，准备写入密文==============*/
            if (!Update.begin((size_t)total)) {
                Serial.print("[OTA] 准备更新失败："); Serial.println(Update.errorString());
                http.end();
                vTaskDelay(pdMS_TO_TICKS(10000)); continue;
            }
            /*==============OTA：初始化AES流式解密和SHA256=================*/
            ota_aes_stream_ctx_t aes_ctx;//定义AES流式解密上下文，保存解密中间状态、缓存块
            const uint8_t *key = ota_aes_get_key();//获取头文件中定义的16字节AES密钥
            const uint8_t *iv  = ota_aes_get_iv();//获取AES-CBC初始向量IV
            ota_aes_stream_init(&aes_ctx, key, iv);//初始化AES流式解密上下文，绑定密钥与IV，准备分段解密密文

            ota_sha256_init(&g_sha_ctx);
            g_ota_written = 0;
            /*==============OTA：流式读取、解密=================*/
            uint8_t buf[128];//128字节临时缓冲区，每次从网络读取密文
            WiFiClient *s = http.getStreamPtr();//获取HTTP底层TCP数据流指针，用来流式读取二进制固件
            size_t total_read = 0;//累计已经处理过的密文字节总数，用来判断是否下载完整固件

            //循环条件：网络连接未断开 并且 处理字节还没达到固件总大小total
            while (http.connected() && total_read < (size_t)total) {
                size_t avail = s->available();
                if (avail == 0) { vTaskDelay(1); continue; }        //暂无数据，短暂让出CPU，不占用资源死等
                if (avail > sizeof(buf)) avail = sizeof(buf);       //限制单次读取不超过buf容量128，防止缓冲区溢出
                int n = s->readBytes(buf, avail);                   //从网络读取avail字节密文存入buf，返回实际读到的字节n
                if (n <= 0) continue;
                
                //AES分组固定16字节，只取出当前分片里能被16整除的完整块用于解密
                size_t aligned = ((size_t)n / 16) * 16;
                if (aligned > 0) {
                    //送入流式AES解密：
                    //aes_ctx：提前初始化好的AES解密上下文
                    //buf：密文缓冲区，aligned：完整16字节块长度
                    //ota_write_cb：解密出明文后，把明文写入OTA Flash分区
                    //ota_sha_cb：同一份明文同步送入SHA256计算哈希
                    if (ota_aes_stream_feed(&aes_ctx, buf, aligned,
                                            ota_write_cb, ota_sha_cb) != 0) {
                        Serial.println("[OTA] AES 解密错误");
                        Update.abort();
                        break;
                    }
                    total_read += aligned;
                }
                //剩下不足16字节的碎片密文，存入aes_ctx内部缓存，等待下一轮数据拼接凑齐16字节再解密
                if (aligned < (size_t)n) {
                    total_read += (n - aligned);
                }
            }
            /*==============OTA：处理最后一块PKCS7填充=================*/
            //判断所有密文数据全部下载完成
            if (total_read == (size_t)total) {
                size_t last_len;
                //AES收尾处理：处理上下文里缓存的不足16字节碎片，去除PKCS7填充
                //解密出最后一段明文，同样通过回调写入Flash、更新SHA哈希
                if (ota_aes_stream_finish(&aes_ctx, &last_len,
                                          ota_write_cb, ota_sha_cb) != 0) {
                    Serial.println("[OTA] AES 收尾错误");
                    Update.abort();
                } else {
                    /*==============OTA：计算 SHA256 并校验=================*/
                    uint8_t hash[32];
                    ota_sha256_final(&g_sha_ctx, hash);//SHA256收尾，输出最终32字节二进制哈希
                    String hex_hash;
                    //把32字节二进制哈希转为64位十六进制字符串
                    for (int i = 0; i < 32; i++) {
                        if (hash[i] < 16) hex_hash += "0";
                        hex_hash += String(hash[i], HEX);
                    }
                    hex_hash.toLowerCase();//统一小写，避免大小写比对失败
                    Serial.print("[OTA] SHA256："); Serial.println(hex_hash);
                    //校验哈希：预期哈希为空（异常）或本地哈希与服务端一致则放行
                    if (expected_hash.length() == 0 || hex_hash == expected_hash) {
                        if (Update.end()) {
                            /* 设置 OTA 回滚验证标志：下次启动先进入验证期 */
                            {
                                Preferences p;
                                p.begin("ota", false);
                                p.putBool("pending", true);     //标记刚升级完，下次开机需要校验固件
                                p.putUChar("attempt", 0);       //开机校验尝试次数清零
                                p.end();
                            }
                            save_local_version(server_ver);
                            Serial.println("[OTA] 升级成功！重启中...");
                            delay(100);
                            esp_restart();      //重启设备加载新固件
                        } else {
                            Serial.print("[OTA] 更新失败："); Serial.println(Update.errorString());
                        }
                    } else {
                        //本地算出的哈希 和 服务器下发哈希不一致，固件被篡改/解密错误
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
