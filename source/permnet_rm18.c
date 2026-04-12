/* ============================================================================
 * permnet_rm18.c -- PermNet-RM(1,8) prototype encoder for HQC-192/HQC-256.
 *
 * Companion to permnet_rm17.c.  Same construction (delta placement + GF(2)
 * zeta-transform butterfly) extended from RM(1,7) with n=128 to RM(1,8) with
 * n=256, k=9.  The 256-bit codeword register is held as four 64-bit "quads"
 * Q0..Q3 covering bit ranges [0..63], [64..127], [128..191], [192..255]
 * respectively, the 9 message bits are placed at the delta positions
 * {0, 1, 2, 4, 8, 16, 32, 64, 128}, and 8 butterfly stages run:
 *   - stages k=0..5 are in-quad and identical in form to permnet_rm17;
 *   - stage  k=6 is between adjacent quad pairs (Q1 ^= Q0; Q3 ^= Q2);
 *   - stage  k=7 is between halves              (Q2 ^= Q0; Q3 ^= Q1).
 *
 * No `-bit` spreading, no message-indexed table lookup, no data-dependent
 * control flow.  Output verified bit-for-bit against the textbook RM(1,8)
 * generator-matrix encoder for every one of the 512 possible 9-bit inputs.
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

void reed_muller_encode_permnet_rm18(uint64_t codeword[4],
                                     const uint16_t *message)
{
    /* ----- Step 1: load 9 message bits as 0/1 values --------------------- */
    const uint64_t mb = (uint64_t)((*message) & 0x1FFu);
    const uint64_t m0 = (mb >> 0) & 1ULL;
    const uint64_t m1 = (mb >> 1) & 1ULL;
    const uint64_t m2 = (mb >> 2) & 1ULL;
    const uint64_t m3 = (mb >> 3) & 1ULL;
    const uint64_t m4 = (mb >> 4) & 1ULL;
    const uint64_t m5 = (mb >> 5) & 1ULL;
    const uint64_t m6 = (mb >> 6) & 1ULL;
    const uint64_t m7 = (mb >> 7) & 1ULL;
    const uint64_t m8 = (mb >> 8) & 1ULL;

    /* ----- Step 2: place at delta positions in the four quads ------------ */
    uint64_t q0 = m0
                | (m1 <<  1)
                | (m2 <<  2)
                | (m3 <<  4)
                | (m4 <<  8)
                | (m5 << 16)
                | (m6 << 32);
    uint64_t q1 = m7;       /* bit 64  = bit 0 of Q1 */
    uint64_t q2 = m8;       /* bit 128 = bit 0 of Q2 */
    uint64_t q3 = 0ULL;

    /* ----- Step 3: six in-quad butterfly stages -------------------------- */
    q0 ^= (q0 & MASK_K0) <<  1;  q1 ^= (q1 & MASK_K0) <<  1;
    q2 ^= (q2 & MASK_K0) <<  1;  q3 ^= (q3 & MASK_K0) <<  1;
    q0 ^= (q0 & MASK_K1) <<  2;  q1 ^= (q1 & MASK_K1) <<  2;
    q2 ^= (q2 & MASK_K1) <<  2;  q3 ^= (q3 & MASK_K1) <<  2;
    q0 ^= (q0 & MASK_K2) <<  4;  q1 ^= (q1 & MASK_K2) <<  4;
    q2 ^= (q2 & MASK_K2) <<  4;  q3 ^= (q3 & MASK_K2) <<  4;
    q0 ^= (q0 & MASK_K3) <<  8;  q1 ^= (q1 & MASK_K3) <<  8;
    q2 ^= (q2 & MASK_K3) <<  8;  q3 ^= (q3 & MASK_K3) <<  8;
    q0 ^= (q0 & MASK_K4) << 16;  q1 ^= (q1 & MASK_K4) << 16;
    q2 ^= (q2 & MASK_K4) << 16;  q3 ^= (q3 & MASK_K4) << 16;
    q0 ^= (q0 & MASK_K5) << 32;  q1 ^= (q1 & MASK_K5) << 32;
    q2 ^= (q2 & MASK_K5) << 32;  q3 ^= (q3 & MASK_K5) << 32;

    /* ----- Step 4: stage k = 6 (shift 64) -------------------------------- */
    q1 ^= q0;
    q3 ^= q2;

    /* ----- Step 5: stage k = 7 (shift 128) ------------------------------- */
    q2 ^= q0;
    q3 ^= q1;

    codeword[0] = q0;
    codeword[1] = q1;
    codeword[2] = q2;
    codeword[3] = q3;
}

/* ========================================================================== *
 *                              Reference + tests
 * ========================================================================== */
static void reed_muller_encode_reference_rm18(uint64_t out[4], uint16_t msg)
{
    msg &= 0x1FFu;
    out[0] = out[1] = out[2] = out[3] = 0;
    for (int j = 0; j < 256; j++) {
        unsigned bit = (msg >> 0) & 1u;     /* m0 always contributes */
        for (int i = 1; i <= 8; i++) {
            unsigned ji = (j >> (i - 1)) & 1u;
            unsigned mi = (msg >> i) & 1u;
            bit ^= (ji & mi);
        }
        int q = j / 64;
        int b = j % 64;
        out[q] |= ((uint64_t)bit) << b;
    }
}

int main(void)
{
    uint16_t msg = 0x1AB;
    uint64_t cw[4];
    reed_muller_encode_permnet_rm18(cw, &msg);
    printf("PermNet-RM(1,8) encode(0x%03X) = "
           "%016llX %016llX %016llX %016llX\n", msg,
           (unsigned long long)cw[3], (unsigned long long)cw[2],
           (unsigned long long)cw[1], (unsigned long long)cw[0]);

    uint64_t r[4];
    reed_muller_encode_reference_rm18(r, msg);
    assert(cw[0] == r[0] && cw[1] == r[1] && cw[2] == r[2] && cw[3] == r[3]);

    for (int b = 0; b < 512; b++) {
        uint16_t mm = (uint16_t)b;
        uint64_t a[4], rr[4];
        reed_muller_encode_permnet_rm18(a, &mm);
        reed_muller_encode_reference_rm18(rr, mm);
        assert(a[0] == rr[0] && a[1] == rr[1] && a[2] == rr[2] && a[3] == rr[3]);
    }
    printf("OK: PermNet-RM(1,8) matches reference for all 512 input messages.\n");
    return 0;
}
