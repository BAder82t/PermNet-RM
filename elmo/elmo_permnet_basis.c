/* ============================================================================
 * elmo_permnet_basis.c -- FAILED EXPERIMENT.
 *
 * Cortex-M0 ELMO harness for the basis-change pair-coupled PermNet-RM(1,7)
 * variant. Kept in the tree as a negative result.
 *
 * Measured on 2026-04-19:
 *   Unmasked PermNet-RM baseline (barrier-protected): bit-6 = 1,757.7
 *   This variant:                                     bit-6 = 3,846.0 (WORSE)
 *
 * See source/permnet_rm17_basis.c for the structural reason (the recovery
 * XOR concentrates the post-butterfly lo1 into a single cycle whose
 * Operand1_data HW is 2-3x larger than any single cycle of the baseline's
 * 1-2-4-8-16-32 ramp).
 *
 * This is the 32-bit counterpart of source/permnet_rm17_basis.c, rewritten
 * to operate on four uint32_t half-words (lo0, lo1, hi0, hi1) in the
 * (u, v, s, t) basis:
 *     u = lo0,  v = lo0 XOR lo1,  s = lo0 XOR hi0,  t = lo0 XOR lo1 XOR hi0 XOR hi1.
 * Because every butterfly stage is GF(2)-linear and applied uniformly to
 * each word, running the standard 5 in-word stages + the cross-word and
 * cross-half stages on (u, v, s, t) and then recovering (lo0, lo1, hi0,
 * hi1) gives identical results to the baseline encoder.
 *
 * The goal is to dilute the bit-6 single-bit Hamming-weight ramp in the
 * 32-bit word `lo1`. In the (u, v, s, t) basis, `v` carries
 * inj_lo0 XOR inj_lo1 = (m0|m1<<1|...|m5<<16) XOR m6, so its stage-0
 * injection has Hamming weight up to 7 rather than 1. The 1-2-4-8-16-32
 * monotone ramp that produces the 1,757.7 bit-6 signal in the baseline
 * binary is broken up.
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

    /* Original-basis injections (only used to derive the (u,v,s,t) basis;
     * never live simultaneously with (u,v,s,t) at runtime after the
     * compiler is done scheduling). */
    uint32_t inj_lo0 = m0 | (m1 << 1) | (m2 << 2) | (m3 << 4)
                     | (m4 << 8) | (m5 << 16);
    uint32_t inj_lo1 = m6;
    uint32_t inj_hi0 = m7;
    uint32_t inj_hi1 = 0;

    /* Basis change: carry (u, v, s, t) through the butterfly. */
    uint32_t u = inj_lo0;
    uint32_t v = inj_lo0 ^ inj_lo1;
    uint32_t s = inj_lo0 ^ inj_hi0;
    uint32_t t = inj_lo0 ^ inj_lo1 ^ inj_hi0 ^ inj_hi1;
    BUTTERFLY_BARRIER(u); BUTTERFLY_BARRIER(v);
    BUTTERFLY_BARRIER(s); BUTTERFLY_BARRIER(t);

    /* Five in-word stages, identical per-word because butterfly is
     * GF(2)-linear and commutes with the basis XOR. */
    u ^= (u & MASK32_K0) << 1;  v ^= (v & MASK32_K0) << 1;
    s ^= (s & MASK32_K0) << 1;  t ^= (t & MASK32_K0) << 1;
    BUTTERFLY_BARRIER(u); BUTTERFLY_BARRIER(v);
    BUTTERFLY_BARRIER(s); BUTTERFLY_BARRIER(t);

    u ^= (u & MASK32_K1) << 2;  v ^= (v & MASK32_K1) << 2;
    s ^= (s & MASK32_K1) << 2;  t ^= (t & MASK32_K1) << 2;
    BUTTERFLY_BARRIER(u); BUTTERFLY_BARRIER(v);
    BUTTERFLY_BARRIER(s); BUTTERFLY_BARRIER(t);

    u ^= (u & MASK32_K2) << 4;  v ^= (v & MASK32_K2) << 4;
    s ^= (s & MASK32_K2) << 4;  t ^= (t & MASK32_K2) << 4;
    BUTTERFLY_BARRIER(u); BUTTERFLY_BARRIER(v);
    BUTTERFLY_BARRIER(s); BUTTERFLY_BARRIER(t);

    u ^= (u & MASK32_K3) << 8;  v ^= (v & MASK32_K3) << 8;
    s ^= (s & MASK32_K3) << 8;  t ^= (t & MASK32_K3) << 8;
    BUTTERFLY_BARRIER(u); BUTTERFLY_BARRIER(v);
    BUTTERFLY_BARRIER(s); BUTTERFLY_BARRIER(t);

    u ^= (u & MASK32_K4) << 16; v ^= (v & MASK32_K4) << 16;
    s ^= (s & MASK32_K4) << 16; t ^= (t & MASK32_K4) << 16;
    BUTTERFLY_BARRIER(u); BUTTERFLY_BARRIER(v);
    BUTTERFLY_BARRIER(s); BUTTERFLY_BARRIER(t);

    /* Stage 5 (cross-word within 64-bit halves): lo1 ^= lo0, hi1 ^= hi0.
     * In the (u,v,s,t) basis with u=lo0, v=lo0^lo1, s=lo0^hi0,
     * t=lo0^lo1^hi0^hi1 this becomes v ^= u; t ^= s. */
    v ^= u;
    t ^= s;
    BUTTERFLY_BARRIER(u); BUTTERFLY_BARRIER(v);
    BUTTERFLY_BARRIER(s); BUTTERFLY_BARRIER(t);

    /* Stage 6 (cross-half): hi ^= lo, i.e. hi0 ^= lo0 and hi1 ^= lo1.
     * Basis: s ^= u; t ^= v. */
    s ^= u;
    t ^= v;
    BUTTERFLY_BARRIER(u); BUTTERFLY_BARRIER(v);
    BUTTERFLY_BARRIER(s); BUTTERFLY_BARRIER(t);

    /* Recovery into the original basis. The critic identified this as the
     * primary risk: each recovery XOR re-materialises a 32-bit word whose
     * Hamming distance against its pre-recovery value is the old
     * post-stage-5 lo1 / hi0 / hi1. Interleave the recoveries with the
     * memory stores so the HD is shared with a memory transaction rather
     * than showing as a clean register-to-register cycle. */
    cw[0] = u;                        /* lo0 = u                        */
    BUTTERFLY_BARRIER(v);
    cw[1] = v ^ u;                    /* lo1 = v ^ u                    */
    BUTTERFLY_BARRIER(s);
    cw[2] = s ^ u;                    /* hi0 = s ^ u                    */
    BUTTERFLY_BARRIER(t);
    cw[3] = t ^ s ^ v ^ u;            /* hi1 = t ^ s ^ v ^ u            */
}

static volatile uint32_t sink;

int main(void)
{
    uint32_t cw[4];
    uint32_t acc = 0;
    int i;

    for (i = 0; i < 256; i++) {
        starttrigger();
        rm_encode_permnet(cw, (uint8_t)i);
        endtrigger();

        acc ^= cw[0] ^ cw[1] ^ cw[2] ^ cw[3];
    }

    sink = acc;
    endprogram();
    return 0;
}
