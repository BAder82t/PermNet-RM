/* ============================================================================
 * permnet_rm17_stage_reordered.c  --  stage-reordered PermNet-RM(1,7) variant
 *
 * WARNING -- this is NOT the interleaved-injection mitigation described in the
 * paper's Section 5.5. It is a distinct and weaker variant that we built and
 * evaluated while exploring that mitigation, and then kept as a negative
 * result so future readers do not rediscover the same dead end.
 *
 * What this variant is
 * --------------------
 * The 7 butterfly stages of the GF(2) zeta transform act on mutually disjoint
 * axes of the Boolean hypercube {0,1}^7. The corresponding linear operators
 * Z_0, ..., Z_6 commute, so any ordering of the seven stages produces the
 * same final codeword (the zeta transform is independent of axis order). This
 * encoder runs them in the order 5, 6, 0, 1, 2, 3, 4. Injection positions are
 * unchanged from the baseline in `permnet_rm17.c`.
 *
 * Why this is NOT a mitigation for the bit-6 isolation effect
 * -----------------------------------------------------------
 * Each within-word butterfly stage is of the form
 *         s ^= (s & MASK_k) << (1 << k);
 * i.e., a left shift only. Cross-word stages k = 5 and k = 6 likewise move
 * bits from `lo0 -> lo1` and from `lo -> hi`, never the other direction.
 *
 * For messages with only m6 or only m7 set, m6 starts at position 32 (bit 0
 * of `lo1`) alone and m7 at position 64 (bit 0 of `hi0`) alone. The cross-
 * word stages cannot pull those bits BACK into `lo0` or into each other,
 * because they are left-shifts. Running those stages earlier only copies the
 * isolated bit into additional empty words -- the original isolated word
 * still propagates its single-bit Hamming-weight signature during the
 * subsequent within-word stages, and one of the two mitigation goals (no
 * 32-bit word holds a single message bit during early stages) is not met.
 *
 * For mixed messages with many bits of m0..m5 set, the reordering does move
 * cross-word mixing earlier, but those bits are not the leakage source
 * identified by the paper in the first place (they share `lo0` from stage 0).
 *
 * The paper's proposed interleaved injection requires actually placing
 * message bits at non-standard positions, which changes the linear map that
 * the butterfly implements. Pure stage commutation cannot achieve that. See
 * notes in LIMITATIONS.md and FIXES_APPLIED.md.
 *
 * What this file is good for
 * --------------------------
 * 1. Documents the attempt and the negative result.
 * 2. Serves as a second correctness harness: any change to the baseline that
 *    breaks stage-commutativity will be caught by the exhaustive test below.
 * 3. Provides a per-stage 32-bit Hamming-weight trace (`-v`) that makes the
 *    asymmetry of left-only butterfly stages visible on single-hot inputs.
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

void reed_muller_encode_permnet_stage_reordered(uint64_t codeword[2],
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

    uint64_t low  = m0
                  | (m1 <<  1)
                  | (m2 <<  2)
                  | (m3 <<  4)
                  | (m4 <<  8)
                  | (m5 << 16)
                  | (m6 << 32);
    uint64_t high = m7;

    /* Stage k = 5 first: propagate lo0 into lo1 and hi0 into hi1. */
    low  ^= (low  & MASK_K5) << 32;
    high ^= (high & MASK_K5) << 32;

    /* Stage k = 6: cross-half XOR. After this, every 32-bit physical word
     * holds contributions from multiple message bits. */
    high ^= low;

    /* Stages k = 0 .. 4, identical to the baseline encoder. */
    low  ^= (low  & MASK_K0) << 1;
    high ^= (high & MASK_K0) << 1;

    low  ^= (low  & MASK_K1) << 2;
    high ^= (high & MASK_K1) << 2;

    low  ^= (low  & MASK_K2) << 4;
    high ^= (high & MASK_K2) << 4;

    low  ^= (low  & MASK_K3) << 8;
    high ^= (high & MASK_K3) << 8;

    low  ^= (low  & MASK_K4) << 16;
    high ^= (high & MASK_K4) << 16;

    codeword[0] = low;
    codeword[1] = high;
}

/* ============================================================================
 *                        Baseline PermNet-RM(1,7) encoder
 * ============================================================================
 * Inlined here (from permnet_rm17.c) so the correctness harness in this file
 * is self-contained and cannot drift from the upstream baseline.
 * ============================================================================ */
static void reed_muller_encode_permnet_baseline(uint64_t codeword[2],
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

    uint64_t low  = m0
                  | (m1 <<  1)
                  | (m2 <<  2)
                  | (m3 <<  4)
                  | (m4 <<  8)
                  | (m5 << 16)
                  | (m6 << 32);
    uint64_t high = m7;

    low  ^= (low  & MASK_K0) << 1;  high ^= (high & MASK_K0) << 1;
    low  ^= (low  & MASK_K1) << 2;  high ^= (high & MASK_K1) << 2;
    low  ^= (low  & MASK_K2) << 4;  high ^= (high & MASK_K2) << 4;
    low  ^= (low  & MASK_K3) << 8;  high ^= (high & MASK_K3) << 8;
    low  ^= (low  & MASK_K4) << 16; high ^= (high & MASK_K4) << 16;
    low  ^= (low  & MASK_K5) << 32; high ^= (high & MASK_K5) << 32;
    high ^= low;

    codeword[0] = low;
    codeword[1] = high;
}

/* ============================================================================
 *                               Diagnostics
 * ============================================================================
 * popcount helpers and a per-stage Hamming-weight trace on the four physical
 * 32-bit words {lo0, lo1, hi0, hi1}. Output is printed only in --verbose mode
 * to keep the CI-friendly default output short. The trace is informational
 * only: the real leakage question requires ELMO or hardware measurement.
 * ============================================================================ */
static int popcount32(uint32_t x)
{
    int n = 0;
    while (x) { n += (int)(x & 1u); x >>= 1; }
    return n;
}

static void split32(uint64_t low, uint64_t high,
                    uint32_t *lo0, uint32_t *lo1,
                    uint32_t *hi0, uint32_t *hi1)
{
    *lo0 = (uint32_t)(low  & 0xFFFFFFFFULL);
    *lo1 = (uint32_t)(low  >> 32);
    *hi0 = (uint32_t)(high & 0xFFFFFFFFULL);
    *hi1 = (uint32_t)(high >> 32);
}

static void trace_stage(const char *tag, uint64_t low, uint64_t high)
{
    uint32_t lo0, lo1, hi0, hi1;
    split32(low, high, &lo0, &lo1, &hi0, &hi1);
    printf("  %-16s HW(lo0,lo1,hi0,hi1) = (%2d,%2d,%2d,%2d)\n",
           tag,
           popcount32(lo0), popcount32(lo1),
           popcount32(hi0), popcount32(hi1));
}

/* Re-executes the interleaved encoder step by step, printing the HW of each
 * 32-bit physical word after every stage. */
static void trace_interleaved(uint8_t msg)
{
    const uint64_t mb = (uint64_t)msg;
    uint64_t low  =  ((mb >> 0) & 1ULL)
                  | (((mb >> 1) & 1ULL) <<  1)
                  | (((mb >> 2) & 1ULL) <<  2)
                  | (((mb >> 3) & 1ULL) <<  4)
                  | (((mb >> 4) & 1ULL) <<  8)
                  | (((mb >> 5) & 1ULL) << 16)
                  | (((mb >> 6) & 1ULL) << 32);
    uint64_t high =  ((mb >> 7) & 1ULL);

    printf("msg=0x%02X\n", msg);
    trace_stage("init",     low, high);

    low  ^= (low  & MASK_K5) << 32;
    high ^= (high & MASK_K5) << 32;
    trace_stage("post-k5",  low, high);

    high ^= low;
    trace_stage("post-k6",  low, high);

    low  ^= (low  & MASK_K0) << 1;
    high ^= (high & MASK_K0) << 1;
    trace_stage("post-k0",  low, high);

    low  ^= (low  & MASK_K1) << 2;
    high ^= (high & MASK_K1) << 2;
    trace_stage("post-k1",  low, high);

    low  ^= (low  & MASK_K2) << 4;
    high ^= (high & MASK_K2) << 4;
    trace_stage("post-k2",  low, high);

    low  ^= (low  & MASK_K3) << 8;
    high ^= (high & MASK_K3) << 8;
    trace_stage("post-k3",  low, high);

    low  ^= (low  & MASK_K4) << 16;
    high ^= (high & MASK_K4) << 16;
    trace_stage("post-k4",  low, high);
}

static void trace_baseline(uint8_t msg)
{
    const uint64_t mb = (uint64_t)msg;
    uint64_t low  =  ((mb >> 0) & 1ULL)
                  | (((mb >> 1) & 1ULL) <<  1)
                  | (((mb >> 2) & 1ULL) <<  2)
                  | (((mb >> 3) & 1ULL) <<  4)
                  | (((mb >> 4) & 1ULL) <<  8)
                  | (((mb >> 5) & 1ULL) << 16)
                  | (((mb >> 6) & 1ULL) << 32);
    uint64_t high =  ((mb >> 7) & 1ULL);

    printf("msg=0x%02X\n", msg);
    trace_stage("init",     low, high);

    low  ^= (low  & MASK_K0) << 1;  high ^= (high & MASK_K0) << 1;
    trace_stage("post-k0",  low, high);
    low  ^= (low  & MASK_K1) << 2;  high ^= (high & MASK_K1) << 2;
    trace_stage("post-k1",  low, high);
    low  ^= (low  & MASK_K2) << 4;  high ^= (high & MASK_K2) << 4;
    trace_stage("post-k2",  low, high);
    low  ^= (low  & MASK_K3) << 8;  high ^= (high & MASK_K3) << 8;
    trace_stage("post-k3",  low, high);
    low  ^= (low  & MASK_K4) << 16; high ^= (high & MASK_K4) << 16;
    trace_stage("post-k4",  low, high);
    low  ^= (low  & MASK_K5) << 32; high ^= (high & MASK_K5) << 32;
    trace_stage("post-k5",  low, high);
    high ^= low;
    trace_stage("post-k6",  low, high);
}

int main(int argc, char **argv)
{
    /* Exhaustive correctness: the stage-reordered variant must produce
     * bit-for-bit identical output to the baseline across all 256 messages.
     * (This is a direct check of butterfly-stage commutativity.) */
    for (int b = 0; b < 256; b++) {
        uint8_t  mm = (uint8_t)b;
        uint64_t a[2], r[2];
        reed_muller_encode_permnet_stage_reordered(a, &mm);
        reed_muller_encode_permnet_baseline(r, &mm);
        if (a[0] != r[0] || a[1] != r[1]) {
            fprintf(stderr,
                    "MISMATCH at msg=0x%02X: reordered=(%016llX %016llX) "
                    "baseline=(%016llX %016llX)\n",
                    mm,
                    (unsigned long long)a[1], (unsigned long long)a[0],
                    (unsigned long long)r[1], (unsigned long long)r[0]);
            return 1;
        }
    }
    printf("OK: stage-reordered variant matches baseline for all 256 input bytes.\n");

    if (argc > 1 && argv[1] != NULL &&
        argv[1][0] == '-' && argv[1][1] == 'v') {
        /* Diagnostic trace: compare per-stage HW trajectories on single-hot
         * messages (one message bit set at a time). This is informational; it
         * is NOT a proxy for ELMO or hardware power measurement. */
        printf("\n--- BASELINE per-stage 32-bit HW trajectory ---\n");
        for (int i = 0; i < 8; i++) {
            trace_baseline((uint8_t)(1u << i));
            printf("\n");
        }
        printf("\n--- INTERLEAVED per-stage 32-bit HW trajectory ---\n");
        for (int i = 0; i < 8; i++) {
            trace_interleaved((uint8_t)(1u << i));
            printf("\n");
        }
    }
    return 0;
}
