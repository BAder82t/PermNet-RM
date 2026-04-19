#!/usr/bin/env python3
"""
verify_theorem_4_2.py -- brute-force check of paper Theorem 4.2
(`thm:zeroprobing`) for RM(1,7).

The theorem (see the paper preprint, Section 4.2) claims that the
Hamming weight of the logical n-bit register state R^(1) after
PermNet's first butterfly stage has zero correlation with each
individual message bit a_i, provided the remaining bits are uniform.

This script enumerates all 256 messages (a_0, a_1, ..., a_7), computes
wt(R^(1)) for each, and prints the per-bit conditional means plus the
gap Delta_i = E[wt | a_i = 1] - E[wt | a_i = 0]. A nonzero gap
disproves Corr(wt(R^(1)), a_i) = 0.

See PROOF_NOTES.md at the repo root for the analytical derivation and
the restated theorem that actually holds.
"""
import itertools


def initial_register(a):
    """Inject message a = (a_0, ..., a_7) into a 128-cell register.

    Cell index corresponds to a subset S of {0,...,6}. a_0 lands at
    S = empty (index 0); a_i lands at S = {i-1} (index 2**(i-1)) for
    i = 1..7. All other cells are 0. (Paper Theorem 3.1.)
    """
    r = [0] * 128
    r[0] = a[0]
    for i in range(1, 8):
        r[1 << (i - 1)] = a[i]
    return r


def butterfly_stage_0(r):
    """Apply stage l=0 of the butterfly: for every subset S containing
    coordinate 0, R^(1)_S = R^(0)_S XOR R^(0)_{S \\ {0}}; otherwise
    unchanged. (Paper Proposition 3.2.)"""
    out = list(r)
    for s in range(128):
        if s & 1:
            out[s] = r[s] ^ r[s ^ 1]
    return out


def hamming_weight(r):
    return sum(r)


def main():
    total = 0
    n = 256
    by_bit_mean_if_0 = [0] * 8
    by_bit_mean_if_1 = [0] * 8
    by_bit_count_if_0 = [0] * 8
    by_bit_count_if_1 = [0] * 8

    for msg_idx in range(n):
        a = [(msg_idx >> b) & 1 for b in range(8)]
        r0 = initial_register(a)
        r1 = butterfly_stage_0(r0)
        w = hamming_weight(r1)
        total += w
        for i in range(8):
            if a[i]:
                by_bit_mean_if_1[i] += w
                by_bit_count_if_1[i] += 1
            else:
                by_bit_mean_if_0[i] += w
                by_bit_count_if_0[i] += 1

    print("Overall mean wt(R^(1)) over 256 messages: {:.4f}".format(total / n))
    print()
    print("Per-bit conditional means and gap Delta_i = E[wt|a_i=1] - E[wt|a_i=0]")
    print("------+----------------+----------------+--------")
    print(" i    | E[wt | a_i=0]  | E[wt | a_i=1]  |  Delta_i")
    print("------+----------------+----------------+--------")
    for i in range(8):
        m0 = by_bit_mean_if_0[i] / by_bit_count_if_0[i]
        m1 = by_bit_mean_if_1[i] / by_bit_count_if_1[i]
        delta = m1 - m0
        print(" {:>2d}   | {:>14.4f} | {:>14.4f} | {:>+6.2f}".format(
            i, m0, m1, delta))
    print("------+----------------+----------------+--------")
    print()
    print("Theorem 4.2 (claim): Delta_i = 0 for every i.")
    print("Actual:              nonzero for i = 0 (Delta=1) and i = 2..7 (Delta=2).")
    print("Conclusion:          theorem as stated does not hold for i != 1.")
    print("See PROOF_NOTES.md for the analytical derivation and restatement.")


if __name__ == "__main__":
    main()
