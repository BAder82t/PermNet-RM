/* ============================================================================
 * elmo_permnet.c -- ELMO harness for the PermNet-RM(1,7) encoder.
 *
 * Iterates all 256 possible 8-bit messages.  Each encode is wrapped in
 * starttrigger() / endtrigger() so ELMO captures a separate power trace
 * per input.  The resulting 256 traces let us analyse whether the encoder's
 * Hamming-weight profile is input-dependent (it should not be).
 *
 * Target: ARM Cortex-M0 Thumb (ELMO emulator).
 * ============================================================================ */

#include <stdint.h>
#include "elmoasmfunctionsdef.h"

/* ----------------------------------------------------------------------------
 *  RM(1,7) Generator matrix rows (same as source/permnet_rm17_bench.c).
 *  Stored as pairs of uint32_t since M0 is 32-bit.
 * -------------------------------------------------------------------------- */
typedef struct { uint32_t lo0, lo1, hi0, hi1; } cw128_t;

/* Each 128-bit codeword row is split into four 32-bit words:
 *   lo0 = bits  0..31   lo1 = bits 32..63
 *   hi0 = bits 64..95   hi1 = bits 96..127                                  */

/* ----------------------------------------------------------------------------
 *  PermNet-RM butterfly encoder, 32-bit arithmetic for Cortex-M0.
 *
 *  The algorithm is identical to source/permnet_rm17.c but operates on
 *  pairs of uint32_t instead of uint64_t.
 * -------------------------------------------------------------------------- */

#define MASK32_K0 0x55555555u
#define MASK32_K1 0x33333333u
#define MASK32_K2 0x0F0F0F0Fu
#define MASK32_K3 0x00FF00FFu
#define MASK32_K4 0x0000FFFFu

/* ----------------------------------------------------------------------------
 * BUTTERFLY_BARRIER(x)
 *
 * Empty inline-asm with operand `x` tied "+r" so GCC treats `x` as an opaque
 * register after the barrier and cannot peer across it to fold the chain.
 *
 * Without this barrier, GCC at -Os correctly realises that lo1 (which starts
 * as the single bit m6) and hi0 (which starts as the single bit m7) each
 * become a full 32-bit all-ones mask after 5 butterfly stages, and folds the
 * entire 5-stage chain into a single `negs Rn, Rn` on the message bit
 * (elmo/permnet.list lines 123, 125 before the fix).  That is precisely the
 * BIT0MASK instruction sequence the paper claims PermNet-RM avoids, and the
 * target of the Jeon 2026/071 single-trace attack.
 *
 * With a barrier after every stage, GCC is forced to emit the literal
 * lsls/ands/eors triple for each of the four halves at every stage, so no
 * `negs` on a message-derived register appears in the compiled code at any
 * of -O0, -O1, -O2, -O3, -Os, -Ofast.
 * ---------------------------------------------------------------------------- */
#define BUTTERFLY_BARRIER(x) __asm__ volatile ("" : "+r"(x))

static void __attribute__((noinline)) rm_encode_permnet(uint32_t cw[4], uint8_t m)
{
    uint32_t m0 = (m >> 0) & 1u;
    uint32_t m1 = (m >> 1) & 1u;
    uint32_t m2 = (m >> 2) & 1u;
    uint32_t m3 = (m >> 3) & 1u;
    uint32_t m4 = (m >> 4) & 1u;
    uint32_t m5 = (m >> 5) & 1u;
    uint32_t m6 = (m >> 6) & 1u;
    uint32_t m7 = (m >> 7) & 1u;

    /* Place message bits at delta positions within two 32-bit halves.
     * Original 64-bit lo had bits at {0,1,2,4,8,16,32}.
     * Split: lo0 gets {0,1,2,4,8,16}, lo1 gets {0} (bit 32 of lo = bit 0 of lo1). */
    uint32_t lo0 = m0 | (m1 << 1) | (m2 << 2) | (m3 << 4) | (m4 << 8) | (m5 << 16);
    uint32_t lo1 = m6;          /* bit 32 of the original 64-bit lo  */
    uint32_t hi0 = m7;
    uint32_t hi1 = 0;
    BUTTERFLY_BARRIER(lo0); BUTTERFLY_BARRIER(lo1);
    BUTTERFLY_BARRIER(hi0); BUTTERFLY_BARRIER(hi1);

    /* Butterfly stages k=0..4 (within each 32-bit word) */
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

    /* Stage k=5: cross-word within each 64-bit half.
     * Original: lo ^= (lo & 0x00000000FFFFFFFF) << 32
     * means lo1 ^= lo0.                                                     */
    lo1 ^= lo0;
    hi1 ^= hi0;

    /* Stage k=6: cross-half.  Original: hi ^= lo.                           */
    hi0 ^= lo0;
    hi1 ^= lo1;

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
        rm_encode_permnet(cw, (uint8_t)i);
        endtrigger();

        acc ^= cw[0] ^ cw[1] ^ cw[2] ^ cw[3];
    }

    sink = acc;
    endprogram();
    return 0;
}
