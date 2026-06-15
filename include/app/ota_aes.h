#ifndef APP_OTA_AES_H
#define APP_OTA_AES_H
 
 #include <stdint.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define OTA_AES_BLOCK_SIZE 16
 
 /* ==================== 批量解密接口 ==================== */
 
 /**
  * 对固件数据进行 AES-128-CBC 解密，并去除 PKCS7 填充
  *
  * @param in       加密输入数据
  * @param out      解密输出缓冲区（可与 in 相同）
  * @param len      加密数据长度（必须为 16 的倍数）
  * @param key      16 字节 AES-128 密钥
  * @param iv       16 字节初始向量
  * @param out_len  输出：解密后实际固件长度（去除填充后）
  * @return         0 成功，-1 出错
  */
 int ota_aes_cbc_decrypt(const uint8_t *in, uint8_t *out, size_t len,
                          const uint8_t key[16], const uint8_t iv[16],
                          size_t *out_len);
 
 /* ==================== 流式解密接口（OTA 流水线用） ==================== */
 
 /* 流式上下文，调用方在栈上分配 */
 typedef struct ota_aes_stream_ctx_s {
     uint32_t rk[44];       /* 扩展密钥 */
     uint8_t  prev[16];     /* 前一块密文 */
     uint8_t  pend[16];     /* 待定解密块 */
     int      have_pending; /* 是否有待定块 */
     int      initialized;  /* 密钥已扩展 */
 } ota_aes_stream_ctx_t;
 
 /* 写/哈希回调签名 */
 typedef void (*ota_aes_write_fn)(uint8_t *data, size_t len);
 typedef void (*ota_aes_sha_fn)(const uint8_t *data, size_t len);
 
 /** 初始化流式解密上下文 */
 void ota_aes_stream_init(ota_aes_stream_ctx_t *ctx,
                           const uint8_t key[16], const uint8_t iv[16]);
 
 /**
  * 投喂加密数据（长度必须为 16 的倍数）
  * 内部使用 pending-block 策略，每次解密后通过回调写固件和更新哈希。
  */
 int ota_aes_stream_feed(ota_aes_stream_ctx_t *ctx,
                          const uint8_t *data, size_t len,
                          ota_aes_write_fn write_fn,
                          ota_aes_sha_fn sha_fn);
 
 /** 收尾：处理最后一块的 PKCS7 填充 */
 int ota_aes_stream_finish(ota_aes_stream_ctx_t *ctx, size_t *out_len,
                            ota_aes_write_fn write_fn,
                            ota_aes_sha_fn sha_fn);
 
 /* ==================== 默认密钥获取 ==================== */
 
 /** 获取 ota.h 中配置的 AES-128 密钥 */
 const uint8_t *ota_aes_get_key(void);
 /** 获取 ota.h 中配置的 AES-128 IV */
 const uint8_t *ota_aes_get_iv(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* APP_OTA_AES_H */
