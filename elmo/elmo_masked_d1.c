/* ============================================================================
 * elmo_masked_d1.c -- ELMO harness for the Boolean-masked PermNet-RM(1,7)
 *                     composition at d = 1.
 *
 * For each 8-bit message m in 0..255, draws a pseudo-random first share s0
 * from a fixed-seed linear congruential PRNG, computes s1 = m XOR s0, and
 * encodes (s0, s1) with the Boolean-masked composition:
 *
 *     masked_encode(s0, s1) = E(s0) XOR E(s1),
 *
 * where E is the baseline PermNet-RM(1,7) encoder. Each masked encoding is
 * wrapped in starttrigger() / endtrigger() so ELMO captures one trace per
 * message. The PRNG seed is a compile-time constant so the run is
 * deterministic and reproducible by-byte across machines.
 *
 * Expected per-bit leakage behaviour
 * ----------------------------------
 * Under the d = 1 probing model, every intermediate variable in either
 * share-encoding is a function of a single share. Because s0 is
 * (pseudo-)uniform and independent of m across the 256 iterations, the
 * marginal distribution of any intermediate variable conditional on
 * bit_b(m) is close to the marginal unconditional distribution. The
 * Pearson correlation between bit_b(m) and the power at any cycle in the
 * captured trace should therefore be substantially smaller than for either
 * the unmasked PermNet-RM or the BIT0MASK baseline.
 *
 * Honest scope
 * ------------
 * 1. With ONE random share per message, the harness is close to (but not
 *    exactly) the standard TVLA methodology. Multi-seed sweeps with many
 *    trials per message would give stronger empirical evidence; they are
 *    easy to add if needed but are not in this harness.
 * 2. The PRNG here is a simple LCG. It is there only to break the
 *    correlation between the share value and m; it is NOT cryptographic.
 *    Production use must draw s0 from a real RNG per call.
 * 3. ELMO models the Cortex-M0 micro-architecture. M4 and other targets
 *    may exhibit different leakage profiles; see LIMITATIONS.md.
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

/* Per-stage compiler barrier. Forces GCC to materialise each intermediate
 * in a GPR between stages, blocking the constant-folding that would
 * otherwise reduce the butterfly on an isolated 1-bit register to a single
 * BIT0MASK-style neg instruction. Must be applied after every stage. See
 * source/permnet_rm17.c for the full rationale. */
#define BUTTERFLY_BARRIER(x) __asm__ volatile ("" : "+r"(x))

/* Baseline 32-bit PermNet-RM(1,7) encoder, identical to elmo_permnet.c. */
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

/* Masked composition. The asm compiler barrier between the two inner
 * encodings prevents fusion, matching source/permnet_rm17_masked_d1.c. */
static void __attribute__((noinline))
rm_encode_permnet_masked_d1(uint32_t cw[4], uint8_t s0, uint8_t s1)
{
    uint32_t cw0[4];
    uint32_t cw1[4];

    rm_encode_permnet(cw0, s0);
    __asm__ volatile ("" ::: "memory");
    rm_encode_permnet(cw1, s1);

    cw[0] = cw0[0] ^ cw1[0];
    cw[1] = cw0[1] ^ cw1[1];
    cw[2] = cw0[2] ^ cw1[2];
    cw[3] = cw0[3] ^ cw1[3];
}

/* Force result to memory so the compiler doesn't optimise it away. */
static volatile uint32_t sink;

int main(void)
{
    /* Linear congruential PRNG. Fixed seed so the whole 256-trace batch is
     * deterministic and the resulting ELMO output is bit-identical across
     * re-runs on the same ELMO + Thumb-binary. */
    uint32_t prng = 0xA11C0DE0u;
    uint32_t cw[4];
    uint32_t acc = 0;

    for (int i = 0; i < 256; i++) {
        /* Advance PRNG and derive one 8-bit share per message. Drawing
         * outside the trigger window would add cycles of constant power
         * before the masked encode; keeping it inside means each trace
         * includes the PRNG call, which is consistent across all 256
         * messages and so does not bias per-bit correlations. */
        starttrigger();
        prng = prng * 1664525u + 1013904223u;
        uint8_t s0 = (uint8_t)(prng >> 24);
        uint8_t s1 = (uint8_t)(s0 ^ (uint8_t)i);
        rm_encode_permnet_masked_d1(cw, s0, s1);
        endtrigger();

        acc ^= cw[0] ^ cw[1] ^ cw[2] ^ cw[3];
    }

    sink = acc;
    endprogram();
    return 0;
}
