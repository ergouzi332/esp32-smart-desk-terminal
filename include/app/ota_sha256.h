#ifndef APP_OTA_SHA256_H
#define APP_OTA_SHA256_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t buffer[64];
} ota_sha256_ctx;

void ota_sha256_init(ota_sha256_ctx *ctx);
void ota_sha256_update(ota_sha256_ctx *ctx, const uint8_t *data, size_t len);
void ota_sha256_final(ota_sha256_ctx *ctx, uint8_t hash[32]);

#ifdef __cplusplus
}
#endif

#endif
