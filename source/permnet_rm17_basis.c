/* ============================================================================
 * permnet_rm17_basis.c -- FAILED EXPERIMENT: basis-change pair-coupled variant
 *
 * WARNING -- this variant is WORSE than the baseline unmasked encoder on
 * ELMO Cortex-M0. It is kept in the tree as a documented negative result so
 * future contributors do not rediscover the same dead end.
 *
 * Measured on 2026-04-19, ELMO Cortex-M0 coeffs_M3.txt:
 *   Unmasked PermNet-RM baseline (with per-stage compiler barriers):
 *       bit-6 signal = 1,757.7
 *   This variant (basis-change pair-coupled):
 *       bit-6 signal = 3,846.0   <- 2.2x WORSE
 *
 * The variant tracks an invertible GF(2) basis change (u = lo0,
 * v = lo0 XOR lo1, s = lo0 XOR hi0, t = lo0 XOR lo1 XOR hi0 XOR hi1)
 * through the butterfly to dilute the m6 ramp in word lo1. The final
 * recovery step cw[1] = v ^ u re-materialises the post-butterfly lo1 in
 * a single cycle. ELMO's Cortex-M0 power model (Mather et al.) scores
 * `Operand1_data` on the stored register value, so that single cycle
 * carries the full 32-bit post-butterfly m6 contribution in one `str`.
 * The 5-cycle 1-2-4-8-16-32 ramp of the baseline is gone, but a single
 * cycle with HW ~32 m6-correlated bits takes its place, and the peak
 * correlator amplitude grows.
 *
 * Structural lower bound (conjecture): for any unmasked RM(1,7) encoder
 * that is correct, branch-free, `neg`-free, and uses no randomness, there
 * must exist at least one cycle whose store/operand register holds a
 * value linearly dependent on m6 with fixed m6-correlated Hamming-weight
 * contribution. The bit-6 ELMO signal is therefore bounded below by a
 * constant set by the RM(1,7) code structure and ELMO coefficients. The
 * baseline encoder achieves 1,757.7; no local reshuffling of the
 * unmasked encoder has been demonstrated to drive it lower. For full
 * bit-6 closure, use the shared-output masked d=1 variant in
 * source/permnet_rm17_masked_d1_shared_output.c (measured bit-6 = 120.9).
 *
 * Motivation
 * ----------
 * In the baseline encoder, the 32-bit physical word lo1 starts as a delta
 * function at bit 0 carrying m6 alone. Its Hamming weight evolves
 * monotonically 1 -> 2 -> 4 -> 8 -> 16 -> 32 across the 5 in-word butterfly
 * stages, giving a clean single-bit correlator with amplitude 1,757.7 in
 * ELMO on Cortex-M0. Ditto for hi0 carrying m7 alone.
 *
 * This variant performs an invertible GF(2) basis change at entry:
 *   u = lo0
 *   v = lo0 XOR lo1     <- "lo1 in the (u,v) basis"
 *   s = lo0 XOR hi0     <- "hi0 in the (u,s) basis"
 *   t = lo0 XOR lo1 XOR hi0 XOR hi1
 * and runs the butterfly on (u, v, s, t) instead of (lo0, lo1, hi0, hi1).
 * Because every butterfly stage is GF(2)-linear and applied uniformly per
 * word, the operation commutes with the XOR-combined basis: running stage k
 * on (u, v, s, t) and then recovering (lo0, lo1, hi0, hi1) = (u, v^u, s^u,
 * t^s^v^u) gives identical results to running stage k directly.
 *
 * In the new basis, v's stage-0 injection has Hamming weight up to 7
 * (popcount(lo0 injection) + m6 contribution) rather than 1. The monotonic
 * single-bit ramp on m6 is summed with a message-dependent base of weight
 * up to 6 from m0..m5, breaking the clean signature that the Jeon-style
 * single-trace attack profiles.
 *
 * Known risk
 * ----------
 * The recovery XORs at the end -- lo1 = v ^ u, hi0 = s ^ u, hi1 = t^s^v^u
 * -- re-materialise 32-bit words whose Hamming distance against the
 * pre-recovery registers equals the original post-stage-5 lo1 / hi0 / hi1.
 * ELMO may show the bit-6 peak moved to that recovery cycle rather than
 * eliminated. The ELMO harness for this variant must measure per-cycle
 * bit-6 signal to confirm the peak drops rather than just moving.
 * ============================================================================ */

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#define MASK_K0 0x5555555555555555ULL
#define MASK_K1 0x3333333333333333ULL
#define MASK_K2 0x0F0F0F0F0F0F0F0FULL
#define MASK_K3 0x00FF00FF00FF00FFULL
#define MASK_K4 0x0000FFFF0000FFFFULL
#define MASK_K5 0x00000000FFFFFFFFULL

#define BUTTERFLY_BARRIER(x) __asm__ volatile ("" : "+r"(x))

void reed_muller_encode_permnet_basis(uint64_t codeword[2],
                                      const uint8_t *message)
{
    const uint64_t mb = (uint64_t)message[0];
    const uint64_t m0 = (mb >> 0) & 1ULL;
    const uint64_t m1 = (mb >> 1) & 1ULL;
    const uint64_t m2 = (mb >> 2) & 1ULL;
    const uint64_t m3 = (mb >> 3) & 1ULL;
    const uint64_t m4 = (mb >> 4) & 1ULL;
    const uint64_t m5 = (mb >> 5) & 1ULL;
    const uint64_t m6 = (mb >> 6) & 1ULL;
    const uint64_t m7 = (mb >> 7) & 1ULL;

    /* Injection values in the original (lo, hi) basis. */
    const uint64_t inj_lo = m0 | (m1 <<  1) | (m2 <<  2) | (m3 <<  4)
                          | (m4 <<  8) | (m5 << 16) | (m6 << 32);
    const uint64_t inj_hi = m7;

    /* Basis change at entry: u = lo, v = hi. For a 64-bit encoder on x86-64
     * the isolated-bit effect does not arise (lo already carries 7 message
     * bits), so the basis we want for maximum dilution is simply the natural
     * one -- no change. However, to exercise the pair-coupled structure
     * that matters for the 32-bit variant, we track the pair (u, v) = (lo,
     * hi). On Cortex-M0 the 32-bit equivalent of this code tracks
     * (u, v, s, t) explicitly; see elmo/elmo_permnet_basis.c. */
    uint64_t u = inj_lo;
    uint64_t v = inj_hi;
    BUTTERFLY_BARRIER(u); BUTTERFLY_BARRIER(v);

    u ^= (u & MASK_K0) << 1;  v ^= (v & MASK_K0) << 1;
    BUTTERFLY_BARRIER(u); BUTTERFLY_BARRIER(v);
    u ^= (u & MASK_K1) << 2;  v ^= (v & MASK_K1) << 2;
    BUTTERFLY_BARRIER(u); BUTTERFLY_BARRIER(v);
    u ^= (u & MASK_K2) << 4;  v ^= (v & MASK_K2) << 4;
    BUTTERFLY_BARRIER(u); BUTTERFLY_BARRIER(v);
    u ^= (u & MASK_K3) << 8;  v ^= (v & MASK_K3) << 8;
    BUTTERFLY_BARRIER(u); BUTTERFLY_BARRIER(v);
    u ^= (u & MASK_K4) << 16; v ^= (v & MASK_K4) << 16;
    BUTTERFLY_BARRIER(u); BUTTERFLY_BARRIER(v);
    u ^= (u & MASK_K5) << 32; v ^= (v & MASK_K5) << 32;
    BUTTERFLY_BARRIER(u); BUTTERFLY_BARRIER(v);
    v ^= u;
    BUTTERFLY_BARRIER(u); BUTTERFLY_BARRIER(v);

    codeword[0] = u;
    codeword[1] = v;
}

/* ============================================================================
 *                     Baseline reference (inlined)
 * ============================================================================ */
static void reed_muller_encode_reference(uint64_t out[2], uint8_t msg)
{
    out[0] = 0;
    out[1] = 0;
    for (int j = 0; j < 128; j++) {
        unsigned bit = (msg >> 0) & 1u;
        for (int i = 1; i <= 7; i++) {
            unsigned ji = (j >> (i - 1)) & 1u;
            unsigned mi = (msg >> i) & 1u;
            bit ^= (ji & mi);
        }
        if (j < 64) out[0] |= ((uint64_t)bit) << j;
        else        out[1] |= ((uint64_t)bit) << (j - 64);
    }
}

int main(void)
{
    for (int b = 0; b < 256; b++) {
        uint8_t  mm = (uint8_t)b;
        uint64_t a[2], r[2];
        reed_muller_encode_permnet_basis(a, &mm);
        reed_muller_encode_reference(r, mm);
        if (a[0] != r[0] || a[1] != r[1]) {
            fprintf(stderr,
                    "MISMATCH at b=0x%02X: basis=(%016llX %016llX) ref=(%016llX %016llX)\n",
                    mm,
                    (unsigned long long)a[1], (unsigned long long)a[0],
                    (unsigned long long)r[1], (unsigned long long)r[0]);
            return 1;
        }
    }
    printf("OK: basis-change variant matches RM(1,7) reference for all 256 inputs.\n");
    return 0;
}
