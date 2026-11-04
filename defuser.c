#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    uint32_t lo = (uint32_t)challenge;
    uint32_t hi = (uint32_t)(challenge >> 32);
    uint32_t mid = (uint32_t)(challenge >> 16);

    uint32_t step1 = (hi + 2u * mid) ^ 0xbbc4u;
    uint32_t step0 = (3u * lo) ^ (hi + 0xf95fu);
    values[0] = 4u * step0 + (step1 ^ 0xe6a7u);
    size_t count = 1;

    return write_answer(out, cap, values, count);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    values[0] = (uint32_t)((challenge >> 1) & 31u) + 14u;

    for (uint32_t i = 1; i < 6; ++i) {
        uint32_t bits = (uint32_t)(challenge >> (i + 12)) & 31u;
        values[i] = (bits ^ (2u * values[i - 1])) + i;
    }

    return write_answer(out, cap, values, 6);

}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};

    uint32_t index = (uint32_t)challenge & 7u;
    uint32_t x = (uint32_t)((challenge >> 3) ^ (challenge >> 32));
    uint32_t result;

switch (index) {
    case 0:  // p3_case_6
        result = 3u * x + 0x5c2au;
        break;
    case 1:  // p3_case_3
        result = (x ^ 0x39c1u) + (x >> 2);
        break;
    case 2:  // p3_case_2
        result = 5u * x + 0x2c1cu;
        break;
    case 3:  // p3_case_5
        result = 5u * x + 0x1883u;
        break;
    case 4:  // p3_case_0
        result = 5u * x + 0xe8c6u;
        break;
    case 5:  // p3_case_1
        result = (2u * x) ^ 0x81d7u;
        break;
    case 6:  // p3_case_4
        result = (4u * x) ^ 0xf462u;
        break;
    default: // index == 7, p3_case_7
        result = (x ^ 0x4f24u) + (x >> 1);
        break;
}

    values[0] = index;
    values[1] = result;
    return write_answer(out, cap, values, 2);
}

static uint32_t p4_walk_local(uint32_t target, uint32_t low,
                              uint32_t high, uint32_t salt) {
    uint32_t mid = low + (high - low) / 2u;

    if (mid == target) {
        return (mid + 0x66u) ^ salt;
    }

    uint32_t mixed = mid + salt;
    if (target > mid) {
        uint32_t child = p4_walk_local(target, mid + 1u, high, salt);
        return ((3u * child) ^ mixed) ^ 2u;
    } else {
        uint32_t child = p4_walk_local(target, low, mid - 1u, salt);
        return ((3u * child) ^ mixed) ^ 1u;
    }
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};

    uint32_t target = (uint32_t)challenge & 0x7fu;
    uint32_t salt = (uint32_t)((challenge >> 7) ^ (challenge >> 32));

    values[0] = p4_walk_local(target, 0u, 127u, salt);
    return write_answer(out, cap, values, 1);
}

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};

    /* 节点编号按内存地址从 0x405ba0 起递增。 */
    static const int next[12] = {
        2, 11, 1, 0, 5, -1, 9, 3, 7, 8, 4, 10
    };

    static const uint32_t tags[12] = {
        0x061fu, 0x051au, 0x0842u, 0x011fu,
        0x0b15u, 0x0010u, 0x0206u, 0x0a15u,
        0x031du, 0x044du, 0x092au, 0x070cu
    };

    static const uint32_t data[12] = {
        0x26e5u, 0x6f93u, 0x70acu, 0xb5f0u,
        0xdba0u, 0xc0efu, 0x29a7u, 0x70ccu,
        0x64a0u, 0x58b5u, 0x1d86u, 0xac2eu
    };

    static const int heads[8] = {
        6, 9, 8, 7, 3, 0, 2, 1
    };

    int node = heads[(uint32_t)challenge & 7u];
    uint32_t steps = (uint32_t)(challenge >> 3) & 3u;
    uint32_t acc =
        (uint32_t)((challenge >> 6) ^ (challenge >> 32)) ^ 0xacb8u;

    /* 起点也要处理，因此总共处理 steps + 1 个节点。 */
    for (uint32_t i = 0; i <= steps; ++i) {
        acc = (3u * acc + data[node]) ^ tags[node];
        values[1] = tags[node];

        if (i < steps) {
            node = next[node];
        }
    }

    values[0] = acc;
    return write_answer(out, cap, values, 2);
}

static uint32_t p6_apply_local(uint32_t index, uint32_t x) {
    /* 按实际函数指针表映射，不按 op 编号排列。 */
    switch (index) {
        case 0:
        case 5:  /* p6_op_4 */
            return ((x >> 1) ^ x) + 0xcd6eu;

        case 1:
        case 6:  /* p6_op_5 */
            return (x ^ 0x7182u) + (x << 7);

        case 2:  /* p6_op_1：32 位循环左移 2 位 */
            return ((x << 2) | (x >> 30)) ^ 0xecdcu;

        case 3:  /* p6_op_2：32 位循环右移 7 位 */
            return (x >> 7) | (x << 25);

        case 4:  /* p6_op_3 */
            return (4u * x + 0xe13fu) ^ (3u * x);

        case 7:  /* p6_op_0 */
            return (x + 0x4f65u) ^ (x >> 7);

        default:
            return 0u;
    }
}
int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};

    uint32_t x =
        (uint32_t)((challenge >> 6) ^ (challenge >> 32));
    uint32_t first = (uint32_t)challenge & 7u;
    uint32_t second = (uint32_t)(challenge >> 3) & 7u;

    uint32_t result = p6_apply_local(first, x);
    result ^= 0xc5b3u;
    result = p6_apply_local(second, result);

    values[0] = result + 0x3c4u;
    return write_answer(out, cap, values, 1);
}

int solve_secret(uint64_t challenge, uint64_t context,
                 char *out, size_t cap) {
    uint32_t a = (uint32_t)challenge
               ^ (uint32_t)(challenge >> 32);
    uint32_t b = (uint32_t)context
               ^ (uint32_t)(context >> 32);

    uint32_t v0 = (uint32_t)context & 3u;
    uint32_t mix = (7u * (a ^ b) + 0xea2au) ^ 0x2694u;

    uint32_t v1 = mix
                ^ (v0 * 0xea2au)
                ^ (uint32_t)(context >> 16);

    uint32_t v2 = v1 ^ (b + v0 + 0x2694u + (a >> 5));

    /* 四段各 8 个 A，共 32 个填充字符。 */
    int n = snprintf(
        out, cap,
        "AAAAAAAA" "AAAAAAAA" "AAAAAAAA" "AAAAAAAA"
        "OPEN%" PRIu32 " %" PRIu32 " %" PRIu32,
        v0, v1, v2
    );

    if (n < 0 || (size_t)n >= cap) {
        return 1;
    }
    return 0;
}
