#include "app/ota_sha256.h"
#include <string.h>

/* SHA256算法64轮常量K
 * 算法标准固定值，由自然数立方根小数部分取前32位生成
 * 每一轮压缩运算都会使用对应下标常量*/
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/*32位整数循环右移*/
static uint32_t ror(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

/*初始化SHA256上下文*/
void ota_sha256_init(ota_sha256_ctx *ctx) {
    //SHA256标准初始哈希值
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    // 累计处理字节数清零
    ctx->count = 0;
}

/*处理单个 512bit(64字节) 数据块，核心压缩运算*/
static void process_block(ota_sha256_ctx *ctx, const uint8_t block[64]) {
    // W[64]: 消息扩展数组; a~h: 8个中间运算变量
    uint32_t W[64], a, b, c, d, e, f, g, h, t1, t2;

    // ========== 第一步：将64字节原始块转为16个32位大端数据 W[0]~W[15] ==========
    for (int i = 0; i < 16; i++) {
        // 每4个字节拼接为1个32位整数，网络/哈希标准大端模式
        W[i] = ((uint32_t)block[i*4] << 24) | ((uint32_t)block[i*4+1] << 16) |
               ((uint32_t)block[i*4+2] << 8) | (uint32_t)block[i*4+3];
    }

    // ========== 第二步：消息扩展，由W[0]~W[15]生成 W[16]~W[63] ==========
    for (int i = 16; i < 64; i++) {
        // 逻辑函数 σ0
        uint32_t s0 = ror(W[i-15], 7) ^ ror(W[i-15], 18) ^ (W[i-15] >> 3);
        // 逻辑函数 σ1
        uint32_t s1 = ror(W[i-2], 17) ^ ror(W[i-2], 19) ^ (W[i-2] >> 10);
        // 递推计算扩展项
        W[i] = W[i-16] + s0 + W[i-7] + s1;
    }

    // 加载当前哈希状态到中间变量 a~h
    a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2]; d = ctx->state[3];
    e = ctx->state[4]; f = ctx->state[5]; g = ctx->state[6]; h = ctx->state[7];

    // ========== 第三步：64轮主压缩运算 ==========
    for (int i = 0; i < 64; i++) {
        // 逻辑函数 ∑1
        uint32_t S1 = ror(e, 6) ^ ror(e, 11) ^ ror(e, 25);
        // 选择函数 Ch
        uint32_t ch = (e & f) ^ ((~e) & g);
        // 逻辑函数 ∑0
        uint32_t S0 = ror(a, 2) ^ ror(a, 13) ^ ror(a, 22);
        // 多数函数 Maj
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);

        // 计算两个临时变量
        t1 = h + S1 + ch + K[i] + W[i];
        t2 = S0 + maj;

        // 寄存器轮转更新
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    // ========== 第四步：累加到全局哈希状态 ==========
    // 当前块运算结果 与 上一轮状态叠加，作为下一块的初始值
    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

/*流式更新SHA256（分段传入数据，支持边下载边计算）*/
void ota_sha256_update(ota_sha256_ctx *ctx, const uint8_t *data, size_t len) {
    // 计算当前缓冲区已存字节数（偏移位置）
    size_t idx = ctx->count % 64;
    // 累计总处理字节数
    ctx->count += len;

    // 1. 缓冲区有残留数据，先补齐到64字节再处理
    if (idx) {
        size_t fill = 64 - idx; // 还需要多少字节凑齐一个块
        // 本次数据不够补齐，直接拷贝到缓冲区，等待下一次数据
        if (len < fill) {
            memcpy(ctx->buffer + idx, data, len);
            return;
        }
        // 拷贝数据凑满64字节，处理当前完整块
        memcpy(ctx->buffer + idx, data, fill);
        process_block(ctx, ctx->buffer);
        // 指针偏移、长度扣减，处理剩余数据
        data += fill;
        len -= fill;
    }

    // 2. 循环处理所有完整的64字节数据块
    while (len >= 64) {
        process_block(ctx, data);
        data += 64;
        len -= 64;
    }

    // 3. 最后剩余不足64字节的数据，存入缓冲区，等待final阶段补位
    if (len)
        memcpy(ctx->buffer, data, len);
}

/**
 * @brief 结束SHA256计算、补位、输出最终32字节哈希结果
 * @param ctx SHA256上下文
 * @param hash 输出缓冲区，固定32字节(256bit)
 * 执行标准补位规则 + 处理最后不足64字节的块，最终导出哈希值
 */
void ota_sha256_final(ota_sha256_ctx *ctx, uint8_t hash[32]) {
    // 总比特数 = 总字节数 * 8，SHA256末尾需要写入原始数据长度(64位)
    uint64_t bits = ctx->count * 8;
    size_t idx = ctx->count % 64;

    // 补位第一步：末尾写入二进制 1 (0x80)
    ctx->buffer[idx++] = 0x80;

    // 情况1：写入0x80后，剩余空间不足64位(8字节)存放长度，先补齐当前块并处理
    if (idx > 56) {
        memset(ctx->buffer + idx, 0, 64 - idx); // 剩余字节填0
        process_block(ctx, ctx->buffer);
        idx = 0; // 重置偏移，新开一个块存放长度
    }

    // 情况2：空间充足，填充0直到倒数8字节
    memset(ctx->buffer + idx, 0, 56 - idx);

    // 最后8字节：写入原始数据总比特长度（大端模式）
    for (int i = 0; i < 8; i++)
        ctx->buffer[56 + i] = (uint8_t)(bits >> (56 - i * 8));

    // 处理最后一个完整数据块
    process_block(ctx, ctx->buffer);

    // 将8个32位哈希状态值，转为32字节二进制哈希结果（大端）
    for (int i = 0; i < 8; i++) {
        hash[i*4]   = (uint8_t)(ctx->state[i] >> 24);
        hash[i*4+1] = (uint8_t)(ctx->state[i] >> 16);
        hash[i*4+2] = (uint8_t)(ctx->state[i] >> 8);
        hash[i*4+3] = (uint8_t)(ctx->state[i]);
    }
}