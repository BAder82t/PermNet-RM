/* ============================================================================
 * permnet_rm17.c  --  PermNet-RM(1,7) prototype encoder for HQC-128
 *
 * Side-channel-hardened software encoder for the Reed-Muller RM(1,7) inner
 * code used by HQC-128.  It eliminates the classical
 *     mask_i = -((message >> i) & 1);
 *     codeword ^= mask_i & G_i;
 * pattern that the YODO compiler-induced violations and the Jeon 2026/071
 * single-trace power-analysis attack both target.
 *
 * PermNet-RM realises the generator matrix as a fully unrolled, straight-line
 * butterfly network of fixed XORs and SHIFTs:
 *
 *   - the 8 message bits are *placed* at fixed delta-function positions in a
 *     128-bit register {0, 1, 2, 4, 8, 16, 32, 64};
 *   - 7 butterfly stages run the GF(2) subset-sum (zeta) transform,
 *     each stage being a single, fixed `s ^= (s & CONST_MASK) << CONST_SHIFT`
 *     applied uniformly to the whole register;
 *   - no  -bit  spreading, no  *0xFF , no message-indexed lookup tables,
 *     no message-dependent control flow, no for/while loop over message bits.
 *
 * Why this is mathematically RM(1,7):
 *   The textbook RM(1,7) encoding maps (m0..m7) -> c where
 *       c[j] = m0  XOR  sum_{i=1..7, bit_{i-1}(j)=1} m_i.
 *   This is exactly the GF(2) zeta transform of an "init" vector that has
 *   init[0] = m0 and init[2^(i-1)] = m_i and zeros elsewhere, evaluated at
 *   index j:
 *       sum_{p subset j} init[p]
 *           =  m0  +  sum_{i: bit_{i-1}(j)=1} m_i.
 *   The zeta transform decomposes into 7 in-place butterfly stages, one per
 *   coordinate axis, mirroring exactly the Plotkin (u, u XOR v) tower that
 *   defines RM(1,7) recursively from RM(1,1).
 * ============================================================================ */

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

/* ----------------------------------------------------------------------------
 * Butterfly masks for the in-half stages k = 0..5.
 * At stage k the mask selects bit positions whose k-th bit is CLEAR; shifting
 * those left by 2^k aligns them onto the matching k-th-bit-SET partners, so
 * the single op  `s ^= (s & MASK_Kk) << (1<<k)`  performs every pairwise XOR
 * for that dimension in parallel without ever inspecting a message bit.
 * ---------------------------------------------------------------------------- */
#define MASK_K0 0x5555555555555555ULL  /* ........ 0101 0101  -- pairs */
#define MASK_K1 0x3333333333333333ULL  /* ........ 0011 0011  -- nibble pairs */
#define MASK_K2 0x0F0F0F0F0F0F0F0FULL  /* ........ 0000 1111  -- byte pairs */
#define MASK_K3 0x00FF00FF00FF00FFULL  /* byte/byte pairs */
#define MASK_K4 0x0000FFFF0000FFFFULL  /* word/word pairs */
#define MASK_K5 0x00000000FFFFFFFFULL  /* dword/dword pairs */
/* Stage k = 6 crosses the 64-bit halves and is realised as `high ^= low`. */

/* ----------------------------------------------------------------------------
 * BUTTERFLY_BARRIER(x)
 *
 * Empty inline-asm with `x` as a read-write register operand.  The asm body
 * does nothing, but the "+r" constraint forces GCC to treat `x` as a fresh,
 * opaque value after each barrier: the compiler cannot peer across the
 * boundary to reason algebraically about `x`, so it cannot constant-fold the
 * butterfly chain applied to an isolated 1-bit register down to a single
 * negation (the `negq %rdx` / `sbfx x1, x1, 7, 1` / `negs r1, r1` idiom that
 * Jeon 2026/071 exploits).  With barriers between each stage the compiler is
 * forced to materialise each intermediate state with its real XOR/AND/SHIFT
 * sequence, so no `neg` on a message-derived register is emitted at any
 * GCC optimisation level.
 * ---------------------------------------------------------------------------- */
#define BUTTERFLY_BARRIER(x) __asm__ volatile ("" : "+r"(x))

void reed_muller_encode_permnet(uint64_t codeword[2], const uint8_t *message)
{
    /* ------------------------------------------------------------------------
     * Step 1.  Load each message bit into its own 0/1-valued variable.
     *
     *   - This is NOT bit-to-mask spreading: every m_i holds the value 0 or 1
     *     in bit position 0 only.  No  -m_i ,  no  m_i * 0xFF .
     *   - Doing the loads as 8 distinct `(mb >> i) & 1` extractions guarantees
     *     the compiler cannot fold them into a data-dependent branch.
     * ---------------------------------------------------------------------- */
    const uint64_t mb = (uint64_t)message[0];
    const uint64_t m0 = (mb >> 0) & 1ULL;
    const uint64_t m1 = (mb >> 1) & 1ULL;
    const uint64_t m2 = (mb >> 2) & 1ULL;
    const uint64_t m3 = (mb >> 3) & 1ULL;
    const uint64_t m4 = (mb >> 4) & 1ULL;
    const uint64_t m5 = (mb >> 5) & 1ULL;
    const uint64_t m6 = (mb >> 6) & 1ULL;
    const uint64_t m7 = (mb >> 7) & 1ULL;

    /* ------------------------------------------------------------------------
     * Step 2.  Place the 8 bits at delta-function positions in the 128-bit
     * register.  Positions are the 7 powers of two {1,2,4,8,16,32,64} plus 0.
     *
     *   low  half (bits 0..63 ) gets m0..m6 at {0,1,2,4,8,16,32}
     *   high half (bits 64..127) gets m7 alone at position 64 (= bit 0 of high)
     *
     * After this placement, the register *literally is* the init vector of
     * the RM(1,7) zeta transform.  Every subsequent operation is fixed.
     * ---------------------------------------------------------------------- */
    uint64_t low  = m0
                  | (m1 <<  1)
                  | (m2 <<  2)
                  | (m3 <<  4)
                  | (m4 <<  8)
                  | (m5 << 16)
                  | (m6 << 32);
    uint64_t high = m7;
    BUTTERFLY_BARRIER(low);
    BUTTERFLY_BARRIER(high);

    /* ------------------------------------------------------------------------
     * Step 3.  In-half butterfly stages k = 0..5.
     *
     * Each line is the entire k-th coordinate of the GF(2) zeta transform:
     *     forall j in 0..63 with bit k of j set:
     *         s[j] ^= s[j XOR 2^k]
     * implemented in one straight-line op per half.  Both halves run the
     * identical sequence of 12 operations -- nothing here depends on m at all.
     *
     * The BUTTERFLY_BARRIER after each stage blocks GCC from constant-folding
     * the entire chain (applied to `high`, which starts as the single bit m7)
     * into a single `neg`/`sbfx` that would spread m7 to all 64 bits in one
     * cycle -- the exact idiom the Jeon single-trace attack targets.
     * ---------------------------------------------------------------------- */

    /* k=0 : pair adjacent bits  (Plotkin level 1, RM(1,1) step) */
    low  ^= (low  & MASK_K0) << 1;
    high ^= (high & MASK_K0) << 1;
    BUTTERFLY_BARRIER(low); BUTTERFLY_BARRIER(high);

    /* k=1 : pair 2-bit groups   (Plotkin level 2, RM(1,2) step) */
    low  ^= (low  & MASK_K1) << 2;
    high ^= (high & MASK_K1) << 2;
    BUTTERFLY_BARRIER(low); BUTTERFLY_BARRIER(high);

    /* k=2 : pair 4-bit groups   (Plotkin level 3, RM(1,3) step) */
    low  ^= (low  & MASK_K2) << 4;
    high ^= (high & MASK_K2) << 4;
    BUTTERFLY_BARRIER(low); BUTTERFLY_BARRIER(high);

    /* k=3 : pair bytes          (Plotkin level 4, RM(1,4) step) */
    low  ^= (low  & MASK_K3) << 8;
    high ^= (high & MASK_K3) << 8;
    BUTTERFLY_BARRIER(low); BUTTERFLY_BARRIER(high);

    /* k=4 : pair 16-bit words   (Plotkin level 5, RM(1,5) step) */
    low  ^= (low  & MASK_K4) << 16;
    high ^= (high & MASK_K4) << 16;
    BUTTERFLY_BARRIER(low); BUTTERFLY_BARRIER(high);

    /* k=5 : pair 32-bit dwords  (Plotkin level 6, RM(1,6) step) */
    low  ^= (low  & MASK_K5) << 32;
    high ^= (high & MASK_K5) << 32;
    BUTTERFLY_BARRIER(low); BUTTERFLY_BARRIER(high);

    /* ------------------------------------------------------------------------
     * Step 4.  The single cross-half butterfly stage k = 6, completing
     *          RM(1,7).
     *
     * After stages 0..5 the high half holds the zeta transform of a delta
     * function at its bit 0, which is a constant: 64 parallel copies of m7.
     * The Plotkin top level then says
     *     codeword[high] = u XOR v,   where v is "all-m7", u is the low half.
     * That is exactly  high ^= low.
     * ---------------------------------------------------------------------- */
    high ^= low;

    codeword[0] = low;
    codeword[1] = high;
}


/* ============================================================================
 *                          Reference encoder + tests
 * ============================================================================
 * The reference encoder applies the textbook RM(1,7) generator-matrix
 * definition directly.  It is intentionally simple (and intentionally NOT
 * constant time) -- it exists ONLY to certify that PermNet-RM produces
 * mathematically identical output for every possible 8-bit input.
 * ============================================================================ */
static void reed_muller_encode_reference(uint64_t out[2], uint8_t msg)
{
    out[0] = 0;
    out[1] = 0;
    for (int j = 0; j < 128; j++) {
        unsigned bit = (msg >> 0) & 1u;     /* m0 always contributes */
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
    /* ---- Demo encode of 0xAB ------------------------------------------- */
    uint8_t  msg = 0xAB;        /* binary 1010 1011 */
    uint64_t cw[2];
    reed_muller_encode_permnet(cw, &msg);

    printf("PermNet-RM(1,7)  encode(0x%02X) = %016llX %016llX\n",
           msg,
           (unsigned long long)cw[1],
           (unsigned long long)cw[0]);

    uint64_t ref[2];
    reed_muller_encode_reference(ref, msg);
    printf("Reference        encode(0x%02X) = %016llX %016llX\n",
           msg,
           (unsigned long long)ref[1],
           (unsigned long long)ref[0]);

    assert(cw[0] == ref[0] && cw[1] == ref[1] &&
           "PermNet-RM disagrees with the textbook RM(1,7) for 0xAB");

    /* ---- Exhaustive cross-check over all 256 possible message bytes ---- */
    for (int b = 0; b < 256; b++) {
        uint8_t  mm = (uint8_t)b;
        uint64_t a[2], r[2];
        reed_muller_encode_permnet(a, &mm);
        reed_muller_encode_reference(r, mm);
        assert(a[0] == r[0] && a[1] == r[1]);
    }
    printf("OK: PermNet-RM matches the reference encoder "
           "for all 256 input bytes.\n");
    return 0;
}
