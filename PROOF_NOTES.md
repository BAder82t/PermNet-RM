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

## Where, concretely, does the theorem fail?

Three distinct failure modes, all pinned down by the `Δ_i` table above:

1. **`a_0` at position `∅` is unmasked.** After stage 0, cell `R^(1)_∅`
   still equals `a_0`. Any probe of that single cell reveals `a_0`.
   Contribution to the gap: `Δ_0 = 1`.
2. **`a_i` for `i ≥ 2` duplicates.** After stage 0, cells
   `R^(1)_{i-1}` and `R^(1)_{0, i-1}` both equal `a_i` (the singleton
   survives unchanged; the pair equals `0 ⊕ a_i` via the stage-0
   XOR). Two unmasked single-bit cells per message bit.
   Contribution: `Δ_i = 2` for `i = 2..m`.
3. **Only `a_1` is properly XOR-masked.** Cell `R^(1)_{{0}}` becomes
   `a_0 ⊕ a_1`, which has expected Hamming weight 1/2 regardless of
   either bit. This is the one bit for which the theorem's conclusion
   holds. Contribution: `Δ_1 = 0`.

The common root is structural: PermNet's injection places `a_0` at
`∅` and `a_i` at the singleton `{i-1}`, and stage 0 only XORs subsets
containing coordinate 0 with their `{0}`-deletion. Only the single
pair `({0}, ∅)` crosses this stage's boundary; everything else either
stays untouched or collides into its own duplicate.

## Can the unmasked encoder be fixed so the original theorem holds?

Short answer: **no, not without changing the output distribution or
adding masking**. Here is why.

1. **`a_0` at `∅` is forced by the algebra.** The zeta transform is
   invertible over GF(2) and the GF(2) Möbius inverse of the all-ones
   function (the column of `G` for `a_0`) is the indicator of `∅`.
   Any other placement of `a_0` changes the codeword `a_0` contributes
   to, i.e. changes the output of `reed_muller_encode`. You cannot
   remove the `R^(1)_∅ = a_0` cell without breaking correctness.
2. **Re-ordering butterfly stages does not help.** The 7 stages
   commute because they act on orthogonal hypercube axes (verified in
   `source/permnet_rm17_stage_reordered.c` as an algebraic sanity
   check). Reordering cannot reduce the number of stage-1 single-bit
   cells that arise after any particular stage, because each stage is
   a left shift and cannot pull an already-isolated bit back into a
   mixed cell. This is the same reason stage-reordering did not fix
   the 32-bit bit-6 isolation empirically (see `LIMITATIONS.md`).
3. **Pre-mixing the message bits (apply an invertible `8×8` GF(2)
   map `A` to the injection vector) does not help for this purpose.**
   Any such pre-mix either preserves the single-bit residual
   structure (if `A` is a permutation) or shifts it to a different
   set of message bits (if `A` mixes bits), but the total
   "single-bit positions in `R^(1)`" count remains `2(m+1) - 1` and
   cannot be zero for `m ≥ 1`. Applying the inverse `A^{-1}` to the
   output is also not an option, because the output `G · A · m` is a
   different linear code than `G · m` in general.
4. **Duplicating injections (e.g., put `a_i` at both `{i-1}` and
   `{0, i-1}` for `i ≥ 2`) breaks correctness.** The extra injection
   adds `sum_{k ∈ j, k ≥ 1} a_{k+1}` to `codeword[j]` for every `j`
   with `0 ∈ j`. The resulting map is no longer the RM(1,m)
   generator; the zeta transform is invertible, so the output
   literally encodes a different message.
5. **Post-mixing the codeword (apply an invertible `128 × 128`
   GF(2) map `B` at the end to "un-mix" residual dependencies)
   changes the output bytes. It does not map back to the RM(1,m)
   codeword of the input message, so it is not a drop-in
   replacement.**

The deepest reason: `wt(R^(1))` is the **integer** Hamming weight of
a GF(2)-linear function of the message, and the integer-valued
conditional expectations `E[wt | a_i = 0]` and `E[wt | a_i = 1]`
differ by the **number of GF(2)-linearly-independent cells of
`R^(1)` in which `a_i` appears unmasked**. That count is a
combinatorial property of the injection-plus-stage-0 pattern. The
zeta-butterfly injection pattern forces this count to be 1 for `a_0`
and 2 for each `a_i` with `i ≥ 2`. No rearrangement of the same
linear algebra can drive all of them to 0 simultaneously.

## What does fix it

**Boolean masking at `d = 1`** (paper §4.5). Under masking, each
share's `R^(1)` is a function of a uniformly random share, not of
the plaintext message `m`. The probing-model correlation between
any single-share intermediate value and any `a_i` is exactly zero —
which is the property the original Theorem 4.2 was trying to state.
The ISW composition theorem then gives `1`-probing security at the
encoder level.

The masked encoder is already in the repository
(`source/permnet_rm17_masked_d1.c`), exhaustively verified over all
`256 × 256` share pairs, and benchmarked. It is the structural fix
that makes the (restated) theorem hold for all message bits, not
just `a_1`.

Empirical caveat (from `elmo/RUN_2026-04-19.md`): the masked
composition with a **reconstructed output** leaks the message bit
on the final unmask XOR (bit 6 peak drops only from `3,779.6` to
`3,794.5`, though leaking-cycle fraction collapses from 38% to 3%).
The **shared-output** variant
(`source/permnet_rm17_masked_d1_shared_output.c`) closes this gap:
it returns `(cw_share0, cw_share1)` with
`cw_share0 XOR cw_share1 = E(m)` and performs no unmask XOR inside
the encoder. ELMO measures peak signal `405.6` (11.1× below
BIT0MASK and 9.4× below the reconstructed variant) and bit-6
signal `191.1` (19.8× below unmasked PermNet). Cost: API change —
downstream HQC consumer must hold both `cw[2]` halves until it is
in a region where unmasking is safe.

## Bottom line for the HQC team

- The **code** is unchanged and correct.
- The **Jeon-style `BIT0MASK` leakage** is eliminated by construction
  on x86-64 (no `neg reg` on a 0/1-valued register anywhere in the
  encoder disassembly across six optimisation levels). This is the
  attack path Jeon actually exploits.
- The **empirical reduction on Cortex-M0 ELMO** (4.6× mean, 84% of
  peak) is real and reproducible; see `elmo/RUN_2026-04-19.md`.
- The **unmasked encoder cannot satisfy the paper's original §4.2
  formal claim** — the enumeration above shows why, and the analysis
  in the "Can the unmasked encoder be fixed?" section shows that no
  local rearrangement of the zeta-butterfly structure fixes it.
  Masking (§4.5) is the fix, at the cost of a ~2× encoder runtime
  plus one fresh byte of randomness per call. This is consistent
  with the paper's own masking section and with the masked
  implementation already in the repository.
- For **Cortex-M4** (Jeon's actual target), hardware measurements
  are the next credible step and are independent of the theorem
  question.

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
