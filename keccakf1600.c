// keccakf1600.c
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void keccakf(uint64_t state[25]) {
    const uint64_t RC[24] = {
        0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
        0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
        0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
        0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
        0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
        0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
        0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
        0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
    };

    int i, j, round;
    uint64_t t, bc[5];

    for (round = 0; round < 24; round++) {
        for (i = 0; i < 5; i++)
            bc[i] = state[i] ^ state[i + 5] ^ state[i + 10] ^ state[i + 15] ^ state[i + 20];

        for (i = 0; i < 5; i++) {
            t = bc[(i + 4) % 5] ^ ((bc[(i + 1) % 5] << 1) | (bc[(i + 1) % 5] >> 63));
            for (j = 0; j < 25; j += 5)
                state[j + i] ^= t;
        }

        t = state[1];
        i = 0;
        for (int x = 0; x < 24; x++) {
            static const int pi[24] = {
                10, 7, 11, 17, 18, 3, 5, 16,
                8, 21, 24, 4, 15, 23, 19, 13,
                12, 2, 20, 14, 22, 9, 6, 1
            };
            static const int rot[24] = {
                1, 3, 6, 10, 15, 21, 28, 36,
                45, 55, 2, 14, 27, 41, 56, 8,
                25, 43, 62, 18, 39, 61, 20, 44
            };
            i++;
            bc[0] = state[pi[x]];
            state[pi[x]] = (t << rot[x]) | (t >> (64 - rot[x]));
            t = bc[0];
        }

        for (j = 0; j < 25; j += 5) {
            for (i = 0; i < 5; i++)
                bc[i] = state[j + i];
            for (i = 0; i < 5; i++)
                state[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }

        state[0] ^= RC[round];
    }
}
#ifdef __cplusplus
}
#endif
