/* ============================================================================
 * elmo_masked_d1_shared_output.c
 *
 * ELMO harness for the shared-output d=1 Boolean-masked PermNet-RM(1,7)
 * encoder. Each of the 256 traces encodes two shares under one trigger pair.
 * The trigger window closes BEFORE the codeword is reconstructed, so any
 * subsequent reconstruction (done in the caller, not here) is out of the
 * measured region.
 *
 * This is the intended comparison variant for elmo_masked_d1.c (which keeps
 * the final XOR inside the trigger window and therefore still shows bit-6
 * peak leakage). If the shared-output design is correct, the bit-6 peak
 * should fall substantially because the cycle that carries the reconstructed
 * codeword is no longer captured.
 *
 * Target: ARM Cortex-M0 Thumb (ELMO emulator).
 * ============================================================================ */

#include <stdint.h>
#include "elmoasmfunctionsdef.h"

#define MASK32_K0 0x55555555u
#define MASK32_K1 0x33333333u
#define MASK32_K2 0x0F0F0F0Fu
#define MASK32_K3 0x00FF00FFu
#define MASK32_K4 0x0000FFFFu

/* Per-stage compiler barrier. See elmo/elmo_permnet.c for the rationale. */
#define BUTTERFLY_BARRIER(x) __asm__ volatile ("" : "+r"(x))

static void __attribute__((noinline))
rm_encode_permnet(uint32_t cw[4], uint8_t m)
{
    uint32_t m0 = (m >> 0) & 1u;
    uint32_t m1 = (m >> 1) & 1u;
    uint32_t m2 = (m >> 2) & 1u;
    uint32_t m3 = (m >> 3) & 1u;
    uint32_t m4 = (m >> 4) & 1u;
    uint32_t m5 = (m >> 5) & 1u;
    uint32_t m6 = (m >> 6) & 1u;
    uint32_t m7 = (m >> 7) & 1u;

    uint32_t lo0 = m0 | (m1 << 1) | (m2 << 2) | (m3 << 4)
                 | (m4 << 8) | (m5 << 16);
    uint32_t lo1 = m6;
    uint32_t hi0 = m7;
    uint32_t hi1 = 0;

    BUTTERFLY_BARRIER(lo0); BUTTERFLY_BARRIER(lo1);
    BUTTERFLY_BARRIER(hi0); BUTTERFLY_BARRIER(hi1);

    lo0 ^= (lo0 & MASK32_K0) << 1;   lo1 ^= (lo1 & MASK32_K0) << 1;
    hi0 ^= (hi0 & MASK32_K0) << 1;   hi1 ^= (hi1 & MASK32_K0) << 1;
    BUTTERFLY_BARRIER(lo0); BUTTERFLY_BARRIER(lo1);
    BUTTERFLY_BARRIER(hi0); BUTTERFLY_BARRIER(hi1);

    lo0 ^= (lo0 & MASK32_K1) << 2;   lo1 ^= (lo1 & MASK32_K1) << 2;
    hi0 ^= (hi0 & MASK32_K1) << 2;   hi1 ^= (hi1 & MASK32_K1) << 2;
    BUTTERFLY_BARRIER(lo0); BUTTERFLY_BARRIER(lo1);
    BUTTERFLY_BARRIER(hi0); BUTTERFLY_BARRIER(hi1);

    lo0 ^= (lo0 & MASK32_K2) << 4;   lo1 ^= (lo1 & MASK32_K2) << 4;
    hi0 ^= (hi0 & MASK32_K2) << 4;   hi1 ^= (hi1 & MASK32_K2) << 4;
    BUTTERFLY_BARRIER(lo0); BUTTERFLY_BARRIER(lo1);
    BUTTERFLY_BARRIER(hi0); BUTTERFLY_BARRIER(hi1);

    lo0 ^= (lo0 & MASK32_K3) << 8;   lo1 ^= (lo1 & MASK32_K3) << 8;
    hi0 ^= (hi0 & MASK32_K3) << 8;   hi1 ^= (hi1 & MASK32_K3) << 8;
    BUTTERFLY_BARRIER(lo0); BUTTERFLY_BARRIER(lo1);
    BUTTERFLY_BARRIER(hi0); BUTTERFLY_BARRIER(hi1);

    lo0 ^= (lo0 & MASK32_K4) << 16;  lo1 ^= (lo1 & MASK32_K4) << 16;
    hi0 ^= (hi0 & MASK32_K4) << 16;  hi1 ^= (hi1 & MASK32_K4) << 16;
    BUTTERFLY_BARRIER(lo0); BUTTERFLY_BARRIER(lo1);
    BUTTERFLY_BARRIER(hi0); BUTTERFLY_BARRIER(hi1);

    lo1 ^= lo0;
    hi1 ^= hi0;

    hi0 ^= lo0;
    hi1 ^= lo1;

    cw[0] = lo0;  cw[1] = lo1;
    cw[2] = hi0;  cw[3] = hi1;
}

/* Shared-output masked composition: two independent encodes, no final XOR. */
static void __attribute__((noinline))
rm_encode_permnet_masked_d1_shared_output(
    uint32_t cw_share0[4], uint32_t cw_share1[4],
    uint8_t s0, uint8_t s1)
{
    rm_encode_permnet(cw_share0, s0);
    __asm__ volatile ("" ::: "memory");
    rm_encode_permnet(cw_share1, s1);
}

static volatile uint32_t sink;

int main(void)
{
    uint32_t prng = 0xA11C0DE0u;
    uint32_t cw_share0[4], cw_share1[4];
    uint32_t acc = 0;

    for (int i = 0; i < 256; i++) {
        starttrigger();
        prng = prng * 1664525u + 1013904223u;
        uint8_t s0 = (uint8_t)(prng >> 24);
        uint8_t s1 = (uint8_t)(s0 ^ (uint8_t)i);
        rm_encode_permnet_masked_d1_shared_output(cw_share0, cw_share1, s0, s1);
        endtrigger();

        /* Reconstruction is OUTSIDE the trigger window; the cycles that
         * touch the reconstructed codeword are not captured in the trace.
         * This is the point of the shared-output design. */
        acc ^= cw_share0[0] ^ cw_share0[1] ^ cw_share0[2] ^ cw_share0[3];
        acc ^= cw_share1[0] ^ cw_share1[1] ^ cw_share1[2] ^ cw_share1[3];
    }

    sink = acc;
    endprogram();
    return 0;
}
