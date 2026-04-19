/* ============================================================================
 * permnet_rm17_masked_d1.c  --  Boolean-masked PermNet-RM(1,7), d = 1
 *
 * Encodes a message m given as two Boolean shares (s0, s1) with
 *     m = s0 XOR s1.
 * Each share is encoded independently with the baseline PermNet-RM(1,7)
 * encoder, and the two codewords are XORed to form the output:
 *
 *     masked_encode(s0, s1) = E(s0) XOR E(s1).
 *
 * Correctness relies only on the linearity of E over GF(2), which is immediate
 * from the construction of PermNet-RM as a straight-line XOR/AND/shift network
 * with no message-dependent control flow (E(0) = 0 and E(a XOR b) = E(a) XOR
 * E(b) for all a, b). The exhaustive test below verifies this across all
 * 256 messages and all 256 possible first-share values.
 *
 * Side-channel intent
 * -------------------
 * Under the d = 1 Boolean-probing model, every intermediate value observed by
 * a single probe during one encoder run is a function of one share only. If
 * the share is uniformly random (s0 sampled uniformly from {0, 1}^8 so that
 * s1 = m XOR s0 is also uniform), then the joint distribution of any
 * single-probe observation is independent of m. This file implements only
 * the composition; the randomness source for s0 is not part of this module.
 * In a real deployment, s0 must be drawn from a fresh, cryptographically
 * strong RNG and must never be reused across encoder calls for the same m.
 *
 * Honesty notes
 * -------------
 * 1. "Under the d = 1 probing model" is a formal statement about the
 *    abstract leakage model, not a claim about any specific hardware. Real
 *    32-bit microprocessors can and do leak through micro-architectural
 *    side channels (glitches, transitions, register reuse) that are not
 *    captured by the probing model. Whether the masked composition reduces
 *    the bit-6 / bit-7 signal observed in ELMO or on real Cortex-M4 silicon
 *    is an empirical question. No such measurement is included in this
 *    commit.
 * 2. The masked composition roughly doubles the compute cost of one RM(1,7)
 *    encode and consumes one 8-bit share of fresh randomness per call.
 *    Benchmarks are in source/permnet_rm17_bench.c.
 * ============================================================================ */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

/* ---------------------------------------------------------------------------
 * Inlined copy of the baseline PermNet-RM(1,7) encoder. Kept here verbatim
 * so this file is self-contained and its exhaustive test cannot be affected
 * by accidental drift of the baseline in permnet_rm17.c.
 * ------------------------------------------------------------------------- */
#define MASK_K0 0x5555555555555555ULL
#define MASK_K1 0x3333333333333333ULL
#define MASK_K2 0x0F0F0F0F0F0F0F0FULL
#define MASK_K3 0x00FF00FF00FF00FFULL
#define MASK_K4 0x0000FFFF0000FFFFULL
#define MASK_K5 0x00000000FFFFFFFFULL

static inline void rm_encode_permnet_share(uint64_t cw[2], uint8_t m)
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

/* ---------------------------------------------------------------------------
 * Boolean-masked PermNet-RM(1,7) composition at d = 1.
 *
 * share0, share1 are 1-byte buffers (matching the HQC encoder's ABI). The
 * full message is m = share0[0] XOR share1[0]. Output codeword at `codeword`
 * is E(m), with E denoting the baseline PermNet-RM(1,7) encoder.
 *
 * The two share encodings are run back-to-back with a compiler barrier
 * between them (an inline asm with "memory" clobber) to prevent the compiler
 * from fusing instructions across shares, which would defeat the masking.
 * ------------------------------------------------------------------------- */
void reed_muller_encode_permnet_masked_d1(uint64_t codeword[2],
                                          const uint8_t share0[1],
                                          const uint8_t share1[1])
{
    uint64_t cw0[2];
    uint64_t cw1[2];

    rm_encode_permnet_share(cw0, share0[0]);

    /* Compiler barrier: do not allow the compiler to reorder or merge the
     * two share encodings. This does not produce a runtime barrier on
     * modern pipelined CPUs (the CPU itself is still free to execute
     * instructions out of order), but it does prevent the optimiser from
     * folding redundant computation across shares. */
    __asm__ volatile ("" ::: "memory");

    rm_encode_permnet_share(cw1, share1[0]);

    codeword[0] = cw0[0] ^ cw1[0];
    codeword[1] = cw0[1] ^ cw1[1];
}

/* ============================================================================
 *                               Correctness
 * ============================================================================
 * The masked composition must produce, for every (share0, share1) pair, the
 * same codeword that the baseline produces on the recovered message
 *     m = share0 XOR share1.
 *
 * We enumerate all 65,536 (share0, share1) pairs; this is the full 256 x 256
 * space. If linearity holds exactly (which it must, by inspection of the
 * baseline), every pair matches.
 * ============================================================================ */
static int exhaustive_correctness(void)
{
    int fail = 0;
    for (int s0 = 0; s0 < 256; s0++) {
        for (int s1 = 0; s1 < 256; s1++) {
            uint8_t  share0 = (uint8_t)s0;
            uint8_t  share1 = (uint8_t)s1;
            uint8_t  m      = (uint8_t)(s0 ^ s1);

            uint64_t masked_cw[2];
            uint64_t direct_cw[2];

            reed_muller_encode_permnet_masked_d1(masked_cw, &share0, &share1);
            rm_encode_permnet_share(direct_cw, m);

            if (masked_cw[0] != direct_cw[0] ||
                masked_cw[1] != direct_cw[1]) {
                fprintf(stderr,
                        "MISMATCH at share0=0x%02X share1=0x%02X m=0x%02X: "
                        "masked=(%016llX %016llX) direct=(%016llX %016llX)\n",
                        share0, share1, m,
                        (unsigned long long)masked_cw[1],
                        (unsigned long long)masked_cw[0],
                        (unsigned long long)direct_cw[1],
                        (unsigned long long)direct_cw[0]);
                fail = 1;
                if (fail >= 5) return 1;
            }
        }
    }
    return fail;
}

int main(void)
{
    /* Demo: encode 0xAB via a random split. */
    srand(1);
    uint8_t msg = 0xAB;
    uint8_t s0  = (uint8_t)(rand() & 0xFF);
    uint8_t s1  = (uint8_t)(msg ^ s0);

    uint64_t cw_masked[2];
    uint64_t cw_direct[2];
    reed_muller_encode_permnet_masked_d1(cw_masked, &s0, &s1);
    rm_encode_permnet_share(cw_direct, msg);

    printf("msg=0x%02X split as (s0=0x%02X, s1=0x%02X)\n", msg, s0, s1);
    printf("masked encode  = %016llX %016llX\n",
           (unsigned long long)cw_masked[1],
           (unsigned long long)cw_masked[0]);
    printf("direct encode  = %016llX %016llX\n",
           (unsigned long long)cw_direct[1],
           (unsigned long long)cw_direct[0]);
    assert(cw_masked[0] == cw_direct[0] && cw_masked[1] == cw_direct[1]);

    /* Exhaustive test over all 256 x 256 = 65,536 share pairs. */
    if (exhaustive_correctness() != 0) {
        fprintf(stderr, "FAIL: masked composition disagreed with baseline.\n");
        return 1;
    }

    printf("OK: masked d=1 composition matches baseline for all "
           "65,536 (share0, share1) pairs.\n");
    return 0;
}
