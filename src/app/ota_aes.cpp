#include "app/ota_aes.h"
#include "app/ota.h"
#include <string.h>

/* ==================== S-Box 涓庨€?S-Box ==================== */

static const uint8_t sbox[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static const uint8_t inv_sbox[256] = {
    0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
    0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
    0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
    0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
    0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
    0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
    0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
    0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
    0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
    0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
    0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
    0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
    0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
    0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
    0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
    0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
};

/* RCON 杞父鏁帮紝绱㈠紩瀵瑰簲杞暟 */
static const uint8_t rcon[11] = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

/* ==================== GF(2^8) 涔樻硶锛堢敤浜?InvMixColumns锛?==================== */

static uint8_t gf_mul2(uint8_t a) {
    return (a & 0x80) ? ((a << 1) ^ 0x1b) : (a << 1);
}
static uint8_t gf_mul4(uint8_t a) { return gf_mul2(gf_mul2(a)); }
static uint8_t gf_mul8(uint8_t a) { return gf_mul2(gf_mul4(a)); }
static uint8_t gf_mul9(uint8_t a)  { return gf_mul8(a) ^ a; }
static uint8_t gf_mul11(uint8_t a) { return gf_mul8(a) ^ gf_mul2(a) ^ a; }
static uint8_t gf_mul13(uint8_t a) { return gf_mul8(a) ^ gf_mul4(a) ^ a; }
static uint8_t gf_mul14(uint8_t a) { return gf_mul8(a) ^ gf_mul4(a) ^ gf_mul2(a); }

/* ==================== 瀵嗛挜鎵╁睍 ==================== */

/* 鍔犺浇 4 瀛楄妭涓哄ぇ绔?uint32 */
static uint32_t load32be(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] <<  8) |
           (uint32_t)p[3];
}

/* 淇濆瓨 uint32 涓哄ぇ绔?4 瀛楄妭 */
static void store32be(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >>  8);
    p[3] = (uint8_t)(v);
}

/* RotWord: 4 瀛楄妭寰幆宸︾Щ 1 浣?*/
static uint32_t rot_word(uint32_t w) {
    return (w << 8) | (w >> 24);
}

/* SubWord: 瀵规瘡涓瓧鑺傚簲鐢?S-box */
static uint32_t sub_word(uint32_t w) {
    uint8_t b0 = sbox[(w >> 24) & 0xff];
    uint8_t b1 = sbox[(w >> 16) & 0xff];
    uint8_t b2 = sbox[(w >>  8) & 0xff];
    uint8_t b3 = sbox[ w        & 0xff];
    return ((uint32_t)b0 << 24) | ((uint32_t)b1 << 16) |
           ((uint32_t)b2 <<  8) | (uint32_t)b3;
}

/*
 * 灏?16 瀛楄妭瀵嗛挜鎵╁睍涓?44 瀛楃殑杞瘑閽ユ暟缁?rk[44]
 * AES-128 闇€瑕?11 杞瘑閽ワ紙44 瀛?= 176 瀛楄妭锛? */
static void key_expansion(const uint8_t key[16], uint32_t rk[44]) {
    for (int i = 0; i < 4; i++) {
        rk[i] = load32be(key + i * 4);
    }
    for (int i = 4; i < 44; i++) {
        uint32_t tmp = rk[i - 1];
        if (i % 4 == 0) {
            tmp = sub_word(rot_word(tmp)) ^ ((uint32_t)rcon[i / 4] << 24);
        }
        rk[i] = rk[i - 4] ^ tmp;
    }
}

/* ==================== AES-128 鍗曞潡瑙ｅ瘑 ==================== */

/*
 * 瑙ｅ瘑涓€涓?16 瀛楄妭鍧楋紝浣跨敤鏍囧噯 AES 閫嗙畻娉曪細
 *   AddRoundKey(rk[10])
 *   for round = 9..1:
 *       InvShiftRows 鈫?InvSubBytes 鈫?AddRoundKey(rk[round]) 鈫?InvMixColumns
 *   InvShiftRows 鈫?InvSubBytes 鈫?AddRoundKey(rk[0])
 *
 * 鐘舵€佺煩闃垫寜鍒椾紭鍏堟帓鍒楋細s[r][c] = block[4*c + r]
 * 杞瘑閽?rk[round * 4 + c] 鐨勭 r 瀛楄妭 = 瀛楄妭 (round + c) 瀛楃殑 (3-r) 瀛楄妭浣嶇疆
 */
static void aes_decrypt_block(uint8_t block[16], const uint32_t rk[44]) {
    uint8_t s[4][4]; /* 琛?r锛屽垪 c */
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            s[r][c] = block[r + 4 * c];

    /* 绗?10 杞細浠?AddRoundKey */
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            s[r][c] ^= (uint8_t)(rk[40 + c] >> (24 - r * 8));

    /* 绗?9 杞?~ 绗?1 杞?*/
    for (int round = 9; round >= 1; round--) {
        /* InvShiftRows锛氳 0 涓嶅彉锛岃 1~3 鍙崇Щ */
        { uint8_t t = s[1][3]; s[1][3] = s[1][2]; s[1][2] = s[1][1]; s[1][1] = s[1][0]; s[1][0] = t; }
        { uint8_t t0 = s[2][0], t1 = s[2][1]; s[2][0] = s[2][2]; s[2][2] = t0; s[2][1] = s[2][3]; s[2][3] = t1; }
        { uint8_t t = s[3][0]; s[3][0] = s[3][1]; s[3][1] = s[3][2]; s[3][2] = s[3][3]; s[3][3] = t; }

        /* InvSubBytes */
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                s[r][c] = inv_sbox[s[r][c]];

        /* AddRoundKey */
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                s[r][c] ^= (uint8_t)(rk[round * 4 + c] >> (24 - r * 8));

        /* InvMixColumns */
        for (int c = 0; c < 4; c++) {
            uint8_t a0 = s[0][c], a1 = s[1][c], a2 = s[2][c], a3 = s[3][c];
            s[0][c] = gf_mul14(a0) ^ gf_mul11(a1) ^ gf_mul13(a2) ^ gf_mul9(a3);
            s[1][c] = gf_mul9(a0)  ^ gf_mul14(a1) ^ gf_mul11(a2) ^ gf_mul13(a3);
            s[2][c] = gf_mul13(a0) ^ gf_mul9(a1)  ^ gf_mul14(a2) ^ gf_mul11(a3);
            s[3][c] = gf_mul11(a0) ^ gf_mul13(a1) ^ gf_mul9(a2)  ^ gf_mul14(a3);
        }
    }

    /* 绗?0 杞細InvShiftRows 鈫?InvSubBytes 鈫?AddRoundKey */
    { uint8_t t = s[1][3]; s[1][3] = s[1][2]; s[1][2] = s[1][1]; s[1][1] = s[1][0]; s[1][0] = t; }
    { uint8_t t0 = s[2][0], t1 = s[2][1]; s[2][0] = s[2][2]; s[2][2] = t0; s[2][1] = s[2][3]; s[2][3] = t1; }
    { uint8_t t = s[3][0]; s[3][0] = s[3][1]; s[3][1] = s[3][2]; s[3][2] = s[3][3]; s[3][3] = t; }
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            s[r][c] = inv_sbox[s[r][c]];
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            s[r][c] ^= (uint8_t)(rk[0 + c] >> (24 - r * 8));

    /* 鍐欏洖 block */
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            block[r + 4 * c] = s[r][c];
}

/* ==================== AES-128-CBC 瑙ｅ瘑涓绘帴鍙?==================== */

/*
 * 浠?ota.h锛堝凡 gitignore锛変腑鑾峰彇瀵嗛挜涓?IV
 * ota.h 瀹氫箟瀹忥細OTA_AES_KEY_DATA 鍜?OTA_AES_IV_DATA
 */
static const uint8_t ota_aes_key[16] = OTA_AES_KEY_DATA;
static const uint8_t ota_aes_iv[16]  = OTA_AES_IV_DATA;

int ota_aes_cbc_decrypt(const uint8_t *in, uint8_t *out, size_t len,
                         const uint8_t key[16], const uint8_t iv[16],
                         size_t *out_len) {
    if (len == 0 || len % OTA_AES_BLOCK_SIZE != 0)
        return -1;  /* 闀垮害鏃犳晥 */

    /* 瀵嗛挜鎵╁睍 */
    uint32_t rk[44];
    key_expansion(key, rk);

    /* 涓婁竴鍧楀瘑鏂囷紙鍒濆涓?IV锛?*/
    uint8_t prev[OTA_AES_BLOCK_SIZE];
    memcpy(prev, iv, OTA_AES_BLOCK_SIZE);

    size_t blocks = len / OTA_AES_BLOCK_SIZE;
    for (size_t i = 0; i < blocks; i++) {
        uint8_t block[OTA_AES_BLOCK_SIZE];
        memcpy(block, in + i * OTA_AES_BLOCK_SIZE, OTA_AES_BLOCK_SIZE);

        aes_decrypt_block(block, rk);

        /* CBC锛氳В瀵嗙粨鏋滀笌鍓嶄竴鍧楀瘑鏂囧紓鎴?*/
        for (int j = 0; j < OTA_AES_BLOCK_SIZE; j++)
            out[i * OTA_AES_BLOCK_SIZE + j] = block[j] ^ prev[j];

        /* 淇濆瓨褰撳墠瀵嗘枃浣滀负涓嬩竴杞殑 prev */
        memcpy(prev, in + i * OTA_AES_BLOCK_SIZE, OTA_AES_BLOCK_SIZE);
    }

    /* 鍘婚櫎 PKCS7 濉厖 */
    size_t unpadded = len;
    if (len > 0) {
        uint8_t pad = out[len - 1];
        if (pad >= 1 && pad <= OTA_AES_BLOCK_SIZE) {
            int valid = 1;
            for (size_t j = len - pad; j < len; j++) {
                if (out[j] != pad) { valid = 0; break; }
            }
            if (valid)
                unpadded = len - pad;
        }
    }

    *out_len = unpadded;
    return 0;
}

/* ==================== 娴佸紡瑙ｅ瘑杈呭姪锛堢敤浜?OTA 娴佹按绾匡級 ==================== */

void ota_aes_stream_init(ota_aes_stream_ctx_t *ctx,
                          const uint8_t key[16], const uint8_t iv[16]) {
    key_expansion(key, ctx->rk);
    memcpy(ctx->prev, iv, 16);
    ctx->have_pending = 0;
    ctx->initialized = 1;
}

int ota_aes_stream_feed(ota_aes_stream_ctx_t *ctx, const uint8_t *data, size_t len,
                         ota_aes_write_fn write_fn,
                         ota_aes_sha_fn sha_fn) {
    if (!ctx->initialized) return -1;
    if (len == 0 || len % 16 != 0) return -1;

    size_t blocks = len / 16;
    for (size_t i = 0; i < blocks; i++) {
        const uint8_t *cblock = data + i * 16;

        /* 瑙ｅ瘑褰撳墠瀵嗘枃鍧?*/
        uint8_t dec[16];
        memcpy(dec, cblock, 16);
        aes_decrypt_block(dec, ctx->rk);

        /* CBC锛氫笌 prev 寮傛垨寰楀埌鏄庢枃 */
        uint8_t plain[16];
        for (int j = 0; j < 16; j++)
            plain[j] = dec[j] ^ ctx->prev[j];

        /* 淇濆瓨褰撳墠瀵嗘枃浣滀负涓嬩竴杞殑 prev */
        memcpy(ctx->prev, cblock, 16);

        /* 濡傛灉宸叉湁 pend 鍧楋紝瀹冪幇鍦ㄦ槸瀹夊叏鐨勶紙闈炴渶鍚庝竴鍧楋級 */
        if (ctx->have_pending) {
            write_fn((uint8_t*)ctx->pend, 16);
            sha_fn(ctx->pend, 16);
        }

        /* 褰撳墠鏄庢枃鎴愪负鏂扮殑 pend */
        memcpy(ctx->pend, plain, 16);
        ctx->have_pending = 1;
    }

    return 0;
}

int ota_aes_stream_finish(ota_aes_stream_ctx_t *ctx, size_t *out_len,
                           ota_aes_write_fn write_fn,
                           ota_aes_sha_fn sha_fn) {
    if (!ctx->initialized || !ctx->have_pending) return -1;

    /* 鏈€鍚庝竴鍧楋細鍘婚櫎 PKCS7 濉厖 */
    uint8_t pad_val = ctx->pend[15];
    int pad_ok = (pad_val >= 1 && pad_val <= 16);
    if (pad_ok) {
        for (int j = 16 - pad_val; j < 16; j++) {
            if (ctx->pend[j] != pad_val) { pad_ok = 0; break; }
        }
    }

    size_t real_len = pad_ok ? (16 - pad_val) : 16;
    write_fn((uint8_t*)ctx->pend, real_len);
    sha_fn(ctx->pend, real_len);

    *out_len = real_len;
    return 0;
}

/* 渚挎嵎鍑芥暟锛氱洿鎺ヤ娇鐢?ota.h 涓殑榛樿瀵嗛挜 */
const uint8_t *ota_aes_get_key(void) { return ota_aes_key; }
const uint8_t *ota_aes_get_iv(void)  { return ota_aes_iv; }

