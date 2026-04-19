/* ============================================================================
 * permnet_rm17_masked_d1_shared_output.c
 *
 * Boolean-masked PermNet-RM(1,7) encoder at d = 1 that keeps the output in
 * SHARED FORM. This is the structural fix for the unmask-cycle leakage
 * observed in source/permnet_rm17_masked_d1.c (see elmo/RUN_2026-04-19.md
 * and PROOF_NOTES.md).
 *
 * Why shared output?
 * ------------------
 * The existing masked composition (source/permnet_rm17_masked_d1.c) takes two
 * message shares s0, s1 with m = s0 XOR s1, encodes each share through the
 * baseline encoder E, and returns the reconstructed codeword
 *     cw = E(s0) XOR E(s1) = E(m).
 * The final XOR is unmasked by construction: the cycle that writes cw to
 * memory carries the full message bit in its Hamming weight, and ELMO on
 * Cortex-M0 shows that single cycle alone preserves the bit-6 peak signal
 * (3,794 vs unmasked 3,780). Reconstructing inside the encoder defeats the
 * probing-model guarantee on the output.
 *
 * This variant instead returns the codeword in shared form:
 *     cw_share0 = E(s0),  cw_share1 = E(s1),  with  cw_share0 XOR cw_share1 = E(m).
 * No unmasking happens inside this encoder. Downstream HQC code that consumes
 * this output must operate on the two halves and defer (or altogether avoid)
 * the unmask step until it is in a region where the leak does not matter.
 *
 * Correctness
 * -----------
 * By GF(2)-linearity of the baseline encoder E: for every pair (s0, s1),
 *     E(s0) XOR E(s1) = E(s0 XOR s1).
 * We exhaustively verify this over all 256 * 256 = 65,536 pairs. The test
 * matches what source/permnet_rm17_masked_d1.c's test runs, with the
 * reconstruction step pushed out to the test itself.
 *
 * API surface
 * -----------
 * The caller supplies TWO output buffers (cw_share0, cw_share1). Each is a
 * full-width 128-bit codeword. This is deliberately NOT ABI-compatible with
 * HQC's reed_muller_encode(); integrating this encoder into HQC requires
 * adjusting the consumer to hold both shares. See PROOF_NOTES.md's "What
 * does fix it" section for the design rationale.
 * ============================================================================ */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define MASK_K0 0x5555555555555555ULL
#define MASK_K1 0x3333333333333333ULL
#define MASK_K2 0x0F0F0F0F0F0F0F0FULL
#define MASK_K3 0x00FF00FF00FF00FFULL
#define MASK_K4 0x0000FFFF0000FFFFULL
#define MASK_K5 0x00000000FFFFFFFFULL

/* Baseline PermNet-RM(1,7) encoder (inlined to keep this file self-contained
 * and prevent drift). Identical to the one in permnet_rm17.c. */
static void rm_encode_permnet_share(uint64_t cw[2], uint8_t m)
{
    const uint64_t mb = (uint64_t)m;
    const uint64_t m0 = (mb >> 0) & 1ULL;
    const uint64_t m1 = (mb >> 1) & 1ULL;
    const uint64_t m2 = (mb >> 2) & 1ULL;
    const uint64_t m3 = (mb >> 3) & 1ULL;
    const uint64_t m4 = (mb >> 4) & 1ULL;
    const uint64_t m5 = (mb >> 5) & 1ULL;
    const uint64_t m6 = (mb >> 6) & 1ULL;
    const uint64_t m7 = (mb >> 7) & 1ULL;

    uint64_t lo = m0 | (m1<<1) | (m2<<2) | (m3<<4)
                | (m4<<8) | (m5<<16) | (m6<<32);
    uint64_t hi = m7;

    lo ^= (lo & MASK_K0) << 1;  hi ^= (hi & MASK_K0) << 1;
    lo ^= (lo & MASK_K1) << 2;  hi ^= (hi & MASK_K1) << 2;
    lo ^= (lo & MASK_K2) << 4;  hi ^= (hi & MASK_K2) << 4;
    lo ^= (lo & MASK_K3) << 8;  hi ^= (hi & MASK_K3) << 8;
    lo ^= (lo & MASK_K4) << 16; hi ^= (hi & MASK_K4) << 16;
    lo ^= (lo & MASK_K5) << 32; hi ^= (hi & MASK_K5) << 32;
    hi ^= lo;

    cw[0] = lo; cw[1] = hi;
}

/* Shared-output masked d = 1 encoder.
 *
 * On entry: share0 and share1 are 1-byte buffers with s0 XOR s1 = m.
 * On return: cw_share0 = E(s0), cw_share1 = E(s1). Reconstruct in the
 * caller (where you know whether unmasking is safe) with
 *     cw[k] = cw_share0[k] ^ cw_share1[k]  for k in {0, 1}.
 *
 * An asm compiler barrier between the two share encodings prevents
 * inter-share fusion by the optimiser. */
void reed_muller_encode_permnet_masked_d1_shared_output(
    uint64_t cw_share0[2],
    uint64_t cw_share1[2],
    const uint8_t share0[1],
    const uint8_t share1[1])
{
    rm_encode_permnet_share(cw_share0, share0[0]);

    __asm__ volatile ("" ::: "memory");

    rm_encode_permnet_share(cw_share1, share1[0]);
}

/* ============================================================================
 *                               Correctness
 * ============================================================================
 * For every (s0, s1) pair:
 *     cw_share0 XOR cw_share1 == E(s0 XOR s1).
 * ============================================================================ */
static int exhaustive_correctness(void)
{
    for (int s0 = 0; s0 < 256; s0++) {
        for (int s1 = 0; s1 < 256; s1++) {
            uint8_t  sh0 = (uint8_t)s0;
            uint8_t  sh1 = (uint8_t)s1;
            uint8_t  m   = (uint8_t)(s0 ^ s1);

            uint64_t cw0[2], cw1[2], cw_direct[2];
            reed_muller_encode_permnet_masked_d1_shared_output(
                cw0, cw1, &sh0, &sh1);
            rm_encode_permnet_share(cw_direct, m);

            uint64_t recon_lo = cw0[0] ^ cw1[0];
            uint64_t recon_hi = cw0[1] ^ cw1[1];
            if (recon_lo != cw_direct[0] || recon_hi != cw_direct[1]) {
                fprintf(stderr,
                        "MISMATCH at s0=0x%02X s1=0x%02X m=0x%02X: "
                        "recon=(%016llX %016llX) direct=(%016llX %016llX)\n",
                        sh0, sh1, m,
                        (unsigned long long)recon_hi,
                        (unsigned long long)recon_lo,
                        (unsigned long long)cw_direct[1],
                        (unsigned long long)cw_direct[0]);
                return 1;
            }
        }
    }
    return 0;
}

int main(void)
{
    /* Demo split of 0xAB. */
    srand(1);
    uint8_t msg = 0xAB;
    uint8_t s0  = (uint8_t)(rand() & 0xFF);
    uint8_t s1  = (uint8_t)(msg ^ s0);

    uint64_t cw0[2], cw1[2];
    reed_muller_encode_permnet_masked_d1_shared_output(cw0, cw1, &s0, &s1);

    uint64_t cw_direct[2];
    rm_encode_permnet_share(cw_direct, msg);

    printf("msg=0x%02X split as (s0=0x%02X, s1=0x%02X)\n", msg, s0, s1);
    printf("share0 codeword = %016llX %016llX\n",
           (unsigned long long)cw0[1],
           (unsigned long long)cw0[0]);
    printf("share1 codeword = %016llX %016llX\n",
           (unsigned long long)cw1[1],
           (unsigned long long)cw1[0]);
    printf("XOR             = %016llX %016llX\n",
           (unsigned long long)(cw0[1] ^ cw1[1]),
           (unsigned long long)(cw0[0] ^ cw1[0]));
    printf("direct encode   = %016llX %016llX\n",
           (unsigned long long)cw_direct[1],
           (unsigned long long)cw_direct[0]);
    assert((cw0[0] ^ cw1[0]) == cw_direct[0]);
    assert((cw0[1] ^ cw1[1]) == cw_direct[1]);

    if (exhaustive_correctness() != 0) {
        fprintf(stderr, "FAIL: shared-output composition disagreed with baseline.\n");
        return 1;
    }
    printf("OK: shared-output d=1 masked encoder produces shares whose XOR "
           "matches the baseline for all 65,536 (s0, s1) pairs.\n");
    return 0;
}
