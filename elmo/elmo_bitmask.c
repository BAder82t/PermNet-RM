/* ============================================================================
 * elmo_bitmask.c -- ELMO harness for the classical BIT0MASK RM(1,7) encoder.
 *
 * This is the encoder used by the HQC reference implementation that Jeon et al.
 * (ePrint 2026/071) and Lai et al. (ePrint 2025/2162, YODO) attack.
 *
 * The vulnerable pattern is:
 *     mask = -((uint32_t)((m >> i) & 1));   // 0x00000000 or 0xFFFFFFFF
 *     cw  ^= mask & G[i];
 *
 * The Hamming weight of `mask` leaks the message bit via power consumption.
 *
 * Target: ARM Cortex-M0 Thumb (ELMO emulator).
 * ============================================================================ */

#include <stdint.h>
#include "elmoasmfunctionsdef.h"

/* ----------------------------------------------------------------------------
 *  RM(1,7) Generator matrix rows, stored as four uint32_t per row.
 * -------------------------------------------------------------------------- */
typedef struct { uint32_t lo0, lo1, hi0, hi1; } cw128_t;

static const cw128_t G[8] = {
    /* G0 = all ones */
    { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu },
    /* G1: 0xAAAA... */
    { 0xAAAAAAAAu, 0xAAAAAAAAu, 0xAAAAAAAAu, 0xAAAAAAAAu },
    /* G2: 0xCCCC... */
    { 0xCCCCCCCCu, 0xCCCCCCCCu, 0xCCCCCCCCu, 0xCCCCCCCCu },
    /* G3: 0xF0F0... */
    { 0xF0F0F0F0u, 0xF0F0F0F0u, 0xF0F0F0F0u, 0xF0F0F0F0u },
    /* G4: 0xFF00... */
    { 0xFF00FF00u, 0xFF00FF00u, 0xFF00FF00u, 0xFF00FF00u },
    /* G5: 0xFFFF0000... */
    { 0xFFFF0000u, 0xFFFF0000u, 0xFFFF0000u, 0xFFFF0000u },
    /* G6: 0xFFFFFFFF00000000 in each 64-bit half */
    { 0x00000000u, 0xFFFFFFFFu, 0x00000000u, 0xFFFFFFFFu },
    /* G7: only the high 64-bit half is set */
    { 0x00000000u, 0x00000000u, 0xFFFFFFFFu, 0xFFFFFFFFu },
};

/* ----------------------------------------------------------------------------
 *  Classical BIT0MASK encoder (the vulnerable one).
 *
 *  For each message bit i, computes mask = -(bit_i) which is either
 *  0x00000000 or 0xFFFFFFFF.  The Hamming weight of the mask (0 or 32)
 *  directly leaks the bit value in the power trace.
 * -------------------------------------------------------------------------- */
static void __attribute__((noinline)) rm_encode_bitmask(uint32_t cw[4], uint8_t m)
{
    uint32_t lo0 = 0, lo1 = 0, hi0 = 0, hi1 = 0;
    int i;

    for (i = 0; i < 8; i++) {
        uint32_t mask = 0u - ((uint32_t)((m >> i) & 1u));
        lo0 ^= mask & G[i].lo0;
        lo1 ^= mask & G[i].lo1;
        hi0 ^= mask & G[i].hi0;
        hi1 ^= mask & G[i].hi1;
    }

    cw[0] = lo0;  cw[1] = lo1;
    cw[2] = hi0;  cw[3] = hi1;
}

/* Force result to memory so the compiler doesn't optimise it away.           */
static volatile uint32_t sink;

int main(void)
{
    uint32_t cw[4];
    uint32_t acc = 0;
    int i;

    for (i = 0; i < 256; i++) {
        starttrigger();
        rm_encode_bitmask(cw, (uint8_t)i);
        endtrigger();

        acc ^= cw[0] ^ cw[1] ^ cw[2] ^ cw[3];
    }

    sink = acc;
    endprogram();
    return 0;
}
