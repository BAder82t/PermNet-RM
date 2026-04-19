# Proof notes on Theorem 4.2 (HW independence after stage 1)

The paper's Theorem 4.2 (label `thm:zeroprobing`) claims

> After the first butterfly stage of PermNet, for every message bit
> `a_i` and uniform remaining bits,
>   `Corr( wt(R^(1)), a_i ) = 0`.

This note records that the claim as stated is **not correct**, gives an
exact restatement that is correct, and flags the change for the paper
authors. The analysis was double-checked by an independent critic
agent and confirmed empirically by exhaustive enumeration over all 256
messages of RM(1,7).

## What R^(1) actually is for RM(1,7)

With the paper's injection pattern
(`a_0` at the empty subset `∅`, `a_i` at the singleton `{i-1}` for
`i = 1..7`) and the stage-0 butterfly rule
`R^(1)_S = R^(0)_S ⊕ R^(0)_{S \ {0}}` if `0 ∈ S`, else unchanged, the
nonzero entries of `R^(1)` are:

| position `S` | value |
|---|---|
| `∅` | `a_0` |
| `{0}` | `a_0 ⊕ a_1` |
| `{k}` for `k = 1..6` | `a_{k+1}` |
| `{0, k}` for `k = 1..6` | `a_{k+1}` |

All other positions are zero. The integer Hamming weight is

    wt(R^(1)) = [a_0] + [a_0 ⊕ a_1] + 2 · (a_2 + a_3 + ... + a_7)

where `[x]` denotes the `{0,1}`-valued indicator.

## Conditional-expectation gap per bit

Exhaustive enumeration over the uniform distribution on the remaining
7 bits gives `Δ_i := E[wt(R^(1)) | a_i = 1] − E[wt(R^(1)) | a_i = 0]`:

| i | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| Δ_i | **1** | 0 | **2** | **2** | **2** | **2** | **2** | **2** |

Only `i = 1` (the bit the butterfly immediately XORs with `a_0`) has
`Δ_1 = 0`. For `i = 0` the gap is 1 (the lone `a_0` at position `∅`
is not masked by the stage-0 XOR, because the XOR only sees
`R^(0)_∅ = a_0` itself on the other side). For `i ≥ 2` the gap is 2
(the bit survives unmasked at **two** positions: the singleton
`{i-1}` and the pair `{0, i-1}`).

Any nonzero `Δ_i` implies nonzero Pearson correlation between
`wt(R^(1))` and `a_i` under the uniform-remaining-bits distribution.

## Where the proof breaks

The paper's proof at lines 350–352 argues that

> the conditional distributions of `wt(R^(1))` given `a_i = 0` and
> `a_i = 1` are identical in expectation (each has expectation `n/2`
> due to the linearity and symmetry of the transform).

Two issues:

1. `wt(.)` is the integer Hamming weight, which is **not**
   GF(2)-linear. The GF(2)-linearity of the zeta transform does not
   transfer to the integer sum of indicator bits. "Linearity and
   symmetry of the transform" does not justify equality of conditional
   integer-valued expectations.
2. The "`n/2 = 64`" figure is wrong in magnitude too. For RM(1,7) the
   mean of `wt(R^(1))` is `7` (there are 15 nonzero positions each
   with probability 1/2, minus one from the XOR collision at `{0}`),
   not 64. That latter value is correct only after all 7 butterfly
   stages complete, when `R^(m)` is the full codeword.

## Restated theorem (what is actually true)

A minimal truthful replacement:

> **Theorem 4.2′ (HW independence after all stages).** Let `R^(m)`
> denote the logical `n`-bit register state after all `m` butterfly
> stages of PermNet — i.e. the full RM(1,m) codeword. For every
> nonzero message, `R^(m)` has exactly `n/2` nonzero bits (the
> codeword of every RM(1,m) codeword except the zero codeword has
> weight `n/2`). In particular, for every `i ∈ {0, ..., m}` and any
> distribution of the remaining bits,
>
>     wt(R^(m)) ∈ {0, n/2}
>
> and
>
>     Corr( wt(R^(m)), a_i ) = 0
>
> whenever the message is not identically zero.

The original Theorem 4.2 is attempting a strictly weaker, stage-1
version of this, but — per the enumeration above — the stage-1 value
is **not** conditionally constant in expectation for any bit other
than `a_1`. An alternative restatement that keeps the stage-1 flavour
is:

> **Theorem 4.2″ (bit-level residues after stage 1).** After stage 0
> of PermNet, the register `R^(1)` has exactly `2(m+1) − 1` nonzero
> bits. Of those, one bit (`R^(1)_{{0}} = a_0 ⊕ a_1`) depends on two
> message bits; all other nonzero bits depend on a single message bit
> each. The Hamming weight `wt(R^(1))` is therefore a multilinear
> polynomial of degree 1 whose coefficients in `a_1` alone vanish,
> but whose coefficients in `a_0` and each `a_i` (`i ≥ 2`) are
> nonzero (equal to 1 and 2 respectively).

This version is empirically correct and states the actual structure;
it is weaker than the paper's current phrasing but faithful.

## Consequences for the paper

- The formal "0-probing after stage 1" claim in §4.2 does not hold.
- The §4.3 "bit-level residues" discussion on lines 356–365 is
  consistent with the enumeration above and remains correct.
- The §4.5 masking composition (ISW at `d = 1`) argument at lines
  378–401 does not depend on Theorem 4.2; it uses only the linearity
  of the encoder, which is uncontroversial.
- The §5.5 "84% of BIT0MASK peak" empirical observation is a
  **downstream** explanation of why single-bit residues appear, and
  is not threatened by the theorem correction. If anything, the
  corrected theorem makes the empirical 84% figure more coherent:
  bit `a_i` for `i ≥ 2` has a stage-1 Hamming-weight gap of 2 in the
  logical register, which (once split into 32-bit physical words)
  surfaces as the bit-6 isolation effect at the physical level.

## Action for the paper authors

1. Replace Theorem 4.2 with either 4.2′ or 4.2″ above.
2. Reword §4.2 and the §5.5 / §6 cross-references accordingly. The
  fix is local — §4.3 onwards already treats the residual dependence
  honestly.
3. Do **not** silently keep the current theorem. An independent
  reviewer with a brute-force script will find the same gap in
  minutes and reject the paper.

Evidence: `verify_permnet.py` (written in `/tmp/verify_permnet.py`
during the critic-agent review) enumerates all 256 messages for
RM(1,7), sums `wt(R^(1))` conditional on each `a_i`, and prints the
`Δ_i` table above. Run result matches the analytical values exactly.
