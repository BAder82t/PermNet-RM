/* ============================================================================
 * permnet_rm17_bench.c -- micro-benchmark for the PermNet-RM(1,7) encoder.
 *
 * Compares three RM(1,7) software encoders:
 *
 *   1. permnet      -- the PermNet-RM encoder from permnet_rm17.c
 *                      (straight-line zeta-transform butterfly,
 *                       no -bit, no *0xFF, no message-indexed lookups).
 *
 *   2. bitmask      -- the classical YODO/Jeon-vulnerable encoder used by
 *                      reference HQC implementations:
 *                          for i in 0..7:
 *                              mask = -((m >> i) & 1);
 *                              cw  ^= mask & G_i;
 *
 *   3. branchy      -- a deliberately branched encoder
 *                          for i in 0..7:
 *                              if ((m >> i) & 1) cw ^= G_i;
 *                      (used as the worst-case timing-leak baseline).
 *
 * Reports for each encoder:
 *   - Throughput in M-encode/s
 *   - Median cycles/op (rdtsc)
 *   - Per-input timing distribution across the 256 distinct input bytes,
 *     and the spread (max - min) of medians.  PermNet-RM and bitmask should
 *     show essentially flat distributions; branchy should fan out by ~7x
 *     based on Hamming weight of the input.
 * ============================================================================ */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <x86intrin.h>

/* ============================================================================
 *   Generator matrix rows for RM(1,7), needed by the bitmask + branchy
 *   encoders.  Row i has bit j set iff j has bit (i-1) set (i in 1..7);
 *   row 0 is all-ones.  Stored as two 64-bit halves.
 * ============================================================================ */
typedef struct { uint64_t lo, hi; } cw128_t;

static const cw128_t G[8] = {
    /* G0 = all ones (constant)                                            */
    { 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL },
    /* G1: bit j set iff (j>>0)&1 -- 0xAAAA...                             */
    { 0xAAAAAAAAAAAAAAAAULL, 0xAAAAAAAAAAAAAAAAULL },
    /* G2: bit j set iff (j>>1)&1 -- 0xCCCC...                             */
    { 0xCCCCCCCCCCCCCCCCULL, 0xCCCCCCCCCCCCCCCCULL },
    /* G3: bit j set iff (j>>2)&1 -- 0xF0F0...                             */
    { 0xF0F0F0F0F0F0F0F0ULL, 0xF0F0F0F0F0F0F0F0ULL },
    /* G4: bit j set iff (j>>3)&1                                          */
    { 0xFF00FF00FF00FF00ULL, 0xFF00FF00FF00FF00ULL },
    /* G5: bit j set iff (j>>4)&1                                          */
    { 0xFFFF0000FFFF0000ULL, 0xFFFF0000FFFF0000ULL },
    /* G6: bit j set iff (j>>5)&1                                          */
    { 0xFFFFFFFF00000000ULL, 0xFFFFFFFF00000000ULL },
    /* G7: bit j set iff (j>>6)&1 -- only the high half is set             */
    { 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL },
};

/* ============================================================================
 *   Encoder 1: PermNet-RM (the same code as permnet_rm17.c)
 * ============================================================================ */
#define MASK_K0 0x5555555555555555ULL
#define MASK_K1 0x3333333333333333ULL
#define MASK_K2 0x0F0F0F0F0F0F0F0FULL
#define MASK_K3 0x00FF00FF00FF00FFULL
#define MASK_K4 0x0000FFFF0000FFFFULL
#define MASK_K5 0x00000000FFFFFFFFULL

static inline __attribute__((always_inline))
void rm_encode_permnet(uint64_t cw[2], uint8_t m)
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

    uint64_t lo = m0 | (m1<<1) | (m2<<2) | (m3<<4) | (m4<<8) | (m5<<16) | (m6<<32);
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

/* ============================================================================
 *   Encoder 2: classical bitmask encoder (the YODO/Jeon vulnerable form)
 * ============================================================================ */
static inline __attribute__((always_inline))
void rm_encode_bitmask(uint64_t cw[2], uint8_t m)
{
    uint64_t lo = 0, hi = 0;
    /* Fully unrolled to give it the best possible shake on speed. */
#define STEP(i)                                                                \
    do {                                                                       \
        uint64_t mask = 0ULL - (uint64_t)((m >> (i)) & 1u);                    \
        lo ^= mask & G[(i)].lo;                                                \
        hi ^= mask & G[(i)].hi;                                                \
    } while (0)
    STEP(0); STEP(1); STEP(2); STEP(3);
    STEP(4); STEP(5); STEP(6); STEP(7);
#undef STEP
    cw[0] = lo; cw[1] = hi;
}

/* ============================================================================
 *   Encoder 3: deliberately branched encoder (timing-leak baseline)
 * ============================================================================ */
static __attribute__((noinline))
void rm_encode_branchy(uint64_t cw[2], uint8_t m)
{
    uint64_t lo = 0, hi = 0;
    /* Each branch is intentionally NOT predictable across random inputs;
     * the goal is to expose Hamming-weight-correlated timing.            */
    if ((m >> 0) & 1u) { lo ^= G[0].lo; hi ^= G[0].hi; }
    if ((m >> 1) & 1u) { lo ^= G[1].lo; hi ^= G[1].hi; }
    if ((m >> 2) & 1u) { lo ^= G[2].lo; hi ^= G[2].hi; }
    if ((m >> 3) & 1u) { lo ^= G[3].lo; hi ^= G[3].hi; }
    if ((m >> 4) & 1u) { lo ^= G[4].lo; hi ^= G[4].hi; }
    if ((m >> 5) & 1u) { lo ^= G[5].lo; hi ^= G[5].hi; }
    if ((m >> 6) & 1u) { lo ^= G[6].lo; hi ^= G[6].hi; }
    if ((m >> 7) & 1u) { lo ^= G[7].lo; hi ^= G[7].hi; }
    cw[0] = lo; cw[1] = hi;
}

/* Force the optimizer to commit results to memory.                          */
static volatile uint64_t SINK_LO, SINK_HI;

/* ============================================================================
 *   Equivalence check
 * ============================================================================ */
static int verify_all_three(void)
{
    for (int b = 0; b < 256; b++) {
        uint64_t a[2], c[2], d[2];
        rm_encode_permnet(a, (uint8_t)b);
        rm_encode_bitmask(c, (uint8_t)b);
        rm_encode_branchy(d, (uint8_t)b);
        if (a[0] != c[0] || a[1] != c[1] || a[0] != d[0] || a[1] != d[1]) {
            fprintf(stderr, "MISMATCH for b=0x%02X\n", b);
            return 0;
        }
    }
    return 1;
}

/* ============================================================================
 *   Throughput benchmark.  Iterates a 256-byte input table N times to
 *   amortise loop overhead and to keep the message stream non-constant.
 * ============================================================================ */
typedef void (*encoder_fn)(uint64_t cw[2], uint8_t m);

static double bench_throughput(const char *name, encoder_fn fn,
                               uint64_t iters)
{
    /* Pre-build a pseudo-random message table.  Same seed for every encoder
     * so they all see identical inputs.                                  */
    uint8_t  msgs[256];
    uint32_t s = 0xC0FFEE01u;
    for (int i = 0; i < 256; i++) { s = s*1664525u + 1013904223u; msgs[i] = (uint8_t)(s >> 24); }

    uint64_t cw[2];
    /* Warmup */
    for (uint64_t k = 0; k < 200000; k++) fn(cw, msgs[k & 0xFF]);

    uint64_t t0 = __rdtsc();
    struct timespec w0, w1;
    clock_gettime(CLOCK_MONOTONIC, &w0);

    uint64_t acc_lo = 0, acc_hi = 0;
    for (uint64_t k = 0; k < iters; k++) {
        fn(cw, msgs[k & 0xFF]);
        acc_lo ^= cw[0];
        acc_hi ^= cw[1];
    }

    clock_gettime(CLOCK_MONOTONIC, &w1);
    uint64_t t1 = __rdtsc();

    SINK_LO = acc_lo; SINK_HI = acc_hi;     /* keep loop alive */

    double seconds = (w1.tv_sec - w0.tv_sec) +
                     (w1.tv_nsec - w0.tv_nsec) * 1e-9;
    double mops    = (double)iters / seconds / 1e6;
    double cyc_per = (double)(t1 - t0) / (double)iters;
    double ns_per  = seconds * 1e9 / (double)iters;

    printf("  %-10s | %10.2f Mops/s | %7.2f ns/op | %7.2f cycles/op\n",
           name, mops, ns_per, cyc_per);
    return mops;
}

/* ============================================================================
 *   Constant-time signal: per-input median cycle cost.  For each of the 256
 *   message bytes, runs an inner repeat-loop a fixed number of times,
 *   measures rdtsc cost, and takes the median over K outer trials.
 *
 *   The metric we care about is (max_b - min_b) of the per-input medians:
 *   if the encoder is constant-time then this spread is bounded by noise.
 * ============================================================================ */
#define INNER_REP   2048u
#define OUTER_TRIAL  41u

static int u64cmp(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

static void bench_per_input(const char *name, encoder_fn fn)
{
    uint64_t medians[256];
    uint64_t trials[OUTER_TRIAL];
    uint64_t cw[2];
    uint64_t sink_xor = 0;

    for (int b = 0; b < 256; b++) {
        for (unsigned t = 0; t < OUTER_TRIAL; t++) {
            uint64_t s0 = __rdtsc();
            for (unsigned r = 0; r < INNER_REP; r++) {
                fn(cw, (uint8_t)b);
                sink_xor ^= cw[0] ^ cw[1];
            }
            uint64_t s1 = __rdtsc();
            trials[t] = (s1 - s0) / INNER_REP;
        }
        qsort(trials, OUTER_TRIAL, sizeof(uint64_t), u64cmp);
        medians[b] = trials[OUTER_TRIAL / 2];
    }
    SINK_LO ^= sink_xor;

    /* Summary statistics across the 256 medians. */
    qsort(medians, 256, sizeof(uint64_t), u64cmp);
    uint64_t mn  = medians[0];
    uint64_t mx  = medians[255];
    uint64_t med = medians[128];
    uint64_t p10 = medians[ 26];
    uint64_t p90 = medians[230];

    /* Hamming-weight-bucketed view (only meaningful for 'branchy'). */
    uint64_t hw_sum[9] = {0};
    int      hw_cnt[9] = {0};
    /* (need original-order medians for this; recompute) */
    for (int b = 0; b < 256; b++) {
        unsigned hw = __builtin_popcount((unsigned)b);
        uint64_t v;
        /* Re-measure quickly: 5 trials, take min, just for the HW table. */
        uint64_t tmp[5];
        for (int t = 0; t < 5; t++) {
            uint64_t s0 = __rdtsc();
            for (unsigned r = 0; r < INNER_REP; r++) {
                fn(cw, (uint8_t)b);
                sink_xor ^= cw[0] ^ cw[1];
            }
            uint64_t s1 = __rdtsc();
            tmp[t] = (s1 - s0) / INNER_REP;
        }
        qsort(tmp, 5, sizeof(uint64_t), u64cmp);
        v = tmp[2];
        hw_sum[hw] += v;
        hw_cnt[hw] += 1;
    }
    SINK_HI ^= sink_xor;

    printf("  %-10s | min=%4llu  p10=%4llu  med=%4llu  p90=%4llu  max=%4llu  "
           "spread=%4llu cycles\n",
           name,
           (unsigned long long)mn, (unsigned long long)p10,
           (unsigned long long)med, (unsigned long long)p90,
           (unsigned long long)mx, (unsigned long long)(mx - mn));
    printf("              hamming-weight bucket medians (cycles/op):\n");
    printf("                ");
    for (int h = 0; h <= 8; h++) {
        printf("hw%d=%4.1f  ", h, (double)hw_sum[h] / (double)hw_cnt[h]);
    }
    printf("\n");
}

int main(void)
{
    if (!verify_all_three()) {
        fprintf(stderr, "FATAL: encoder mismatch\n");
        return 1;
    }
    printf("PermNet-RM(1,7) micro-benchmark\n");
    printf("Build: " __DATE__ " " __TIME__ "  compiler="
#ifdef __clang__
           "clang"
#elif defined(__GNUC__)
           "gcc"
#else
           "unknown"
#endif
           "\n");
    printf("All three encoders agree on every 8-bit input.\n\n");

    const uint64_t ITERS = 200ULL * 1000ULL * 1000ULL;
    printf("Throughput  (%llu encodes per run):\n",
           (unsigned long long)ITERS);
    printf("  encoder    |     throughput   |   latency    |   cycles\n");
    printf("  -----------+------------------+--------------+----------\n");
    bench_throughput("permnet",  rm_encode_permnet, ITERS);
    bench_throughput("bitmask",  rm_encode_bitmask, ITERS);
    bench_throughput("branchy",  rm_encode_branchy, ITERS);

    printf("\nPer-input timing (median over %u trials of %u inner reps),\n"
           "256 distinct message bytes, lower spread => more constant-time:\n",
           OUTER_TRIAL, INNER_REP);
    printf("  encoder    | distribution of per-input medians\n");
    printf("  -----------+---------------------------------------------------\n");
    bench_per_input("permnet", rm_encode_permnet);
    bench_per_input("bitmask", rm_encode_bitmask);
    bench_per_input("branchy", rm_encode_branchy);

    return 0;
}
