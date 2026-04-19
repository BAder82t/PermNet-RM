# Proof notes — stage-1 register structure and masking

This note pins down the exact structure of the PermNet-RM logical
register after the first butterfly stage, quantifies the residual
per-bit Hamming-weight dependence, and explains which design choice
removes it.

## Stage-1 register structure (RM(1,7))

With the injection pattern of the encoder (`a_0` at subset `∅`, `a_i`
at the singleton `{i-1}` for `i = 1..7`) and the first butterfly
stage (XOR `R_S ← R_S ⊕ R_{S ∖ {0}}` for subsets `S` with `0 ∈ S`,
identity otherwise), the nonzero cells of `R^(1)` are:

| position `S` | value |
|---|---|
| `∅` | `a_0` |
| `{0}` | `a_0 ⊕ a_1` |
| `{k}` for `k = 1..6` | `a_{k+1}` |
| `{0, k}` for `k = 1..6` | `a_{k+1}` |

Every other cell is zero. The integer Hamming weight is

    wt(R^(1)) = [a_0] + [a_0 ⊕ a_1] + 2 · (a_2 + a_3 + … + a_7)

where `[x]` denotes the `{0,1}`-valued indicator.

## Per-bit residual after stage 1

Let `Δ_i := E[wt(R^(1)) | a_i = 1] − E[wt(R^(1)) | a_i = 0]` with the
other bits uniform. Exhaustive enumeration over all 256 messages:

| i | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| Δ_i | 1 | 0 | 2 | 2 | 2 | 2 | 2 | 2 |

Only `a_1` has `Δ_i = 0` at stage 1. The script
[`source/verify_theorem_4_2.py`](source/verify_theorem_4_2.py) regenerates
this table from scratch.

The structural reason is direct:
- `a_0` sits alone at `∅`; nothing XORs with it at stage 0.
- `a_1` gets mixed with `a_0` at cell `{0}` (stage 0 crosses axis 0).
- `a_i` for `i ≥ 2` occupies two cells unmasked: the original
  singleton `{i-1}` and the pair `{0, i-1}` (stage 0's XOR of `{0,i-1}`
  with `{i-1}` leaves `0 ⊕ a_i = a_i`).

## Can the residual be driven to zero without masking?

Short answer: no, not while keeping the same RM(1,m) output code.
Five families of candidate fixes all fail:

1. **Move `a_0` off position `∅`.** The GF(2) Möbius inverse of the
   RM(1,m) generator's constant column is the indicator of `∅`, so
   placing `a_0` anywhere else changes the codeword.
2. **Reorder the butterfly stages.** The 7 stages commute, so
   reordering does not change the final output; the stage-1 residuals
   are also unchanged in count because each stage is a left shift and
   cannot pull an isolated bit back into a shared cell. Demonstrated
   empirically by [`source/permnet_rm17_stage_reordered.c`](source/permnet_rm17_stage_reordered.c)
   (kept as a documented negative result).
3. **Pre-mix the message with an invertible GF(2) map `A`.** Either
   preserves the single-bit residue structure (if `A` is a
   permutation) or redistributes it across different bits (if `A`
   mixes bits). The count `2(m+1) − 1` of single-bit cells is
   invariant.
4. **Duplicate injections** (inject `a_i` at both `{i-1}` and
   `{0, i-1}`). The zeta transform is GF(2)-linear; the duplication
   adds `sum_{k ∈ j, k ≥ 1} a_{k+1}` to `codeword[j]` for every `j`
   with `0 ∈ j`. The resulting map is no longer the RM(1,m) generator.
5. **Post-mix the codeword with an invertible `128 × 128` map.**
   Changes the output bytes; the result is not the RM(1,m) codeword
   of the input message.

The deepest reason: the per-bit gap `Δ_i` equals the number of
GF(2)-linearly-independent cells of `R^(1)` in which `a_i` appears
unmasked. That count is a combinatorial property of the
injection-plus-stage-0 pattern, and the zeta-butterfly topology
forces it to be 1 for `a_0`, 0 for `a_1`, and 2 for each `a_i` with
`i ≥ 2`. No rearrangement of the same linear algebra can drive all
`Δ_i` to zero simultaneously.

## What does drive the residuals to zero

Boolean masking at `d = 1`
([`source/permnet_rm17_masked_d1.c`](source/permnet_rm17_masked_d1.c)
and its shared-output refinement
[`source/permnet_rm17_masked_d1_shared_output.c`](source/permnet_rm17_masked_d1_shared_output.c)).
Under masking, each share's `R^(1)` is a function of a uniformly
random share alone, not of the plaintext message `m`. The
probing-model correlation between any single-share intermediate
value and any `a_i` is exactly zero. The ISW composition theorem
then gives 1-probing security at the encoder level.

The shared-output variant additionally keeps the codeword in shared
form `(cw_share0, cw_share1)` with `cw_share0 XOR cw_share1 = E(m)`
and performs no unmask XOR inside the encoder. Empirically on
Cortex-M0 ELMO:

| | Peak | Mean | Leaking cycles | Bit 6 |
|---|---:|---:|---:|---:|
| BIT0MASK (baseline) | 4,493.4 | 2,687.0 | 199/293 (68%) | 3,778.4 |
| Unmasked PermNet-RM (with barrier fix) | 1,757.7 | 294.97 | 55/144 (38%) | 1,757.7 |
| Masked d=1, reconstructed | 3,794.5 | 675.9 | 7/211 (3%) | 3,794.5 |
| Masked d=1, **shared output** | **405.6** | **204.6** | **3/184 (1.6%)** | **191.1** |

Shared-output vs BIT0MASK: **11.1× peak reduction, 13.1× mean
reduction, 68% → 1.6% leaking cycles**. Shared-output vs unmasked
PermNet: **19.8× reduction on bit 6** — the specific bit that drove
the residual leakage in the unmasked encoder. Full reproduction
report: [`elmo/RUN_2026-04-19.md`](elmo/RUN_2026-04-19.md).

Cost trade-offs:
- Runtime: ~2× the unmasked encoder.
- Randomness: one fresh byte per call (cryptographic RNG required
  in production).
- API: NOT ABI-compatible with `reed_muller_encode()`. Downstream
  HQC code consumes both shares until reconstruction is safe.

## Bottom line for downstream integration

- The encoder source code is correct; exhaustive correctness is
  verified at six GCC optimisation levels.
- The Jeon-style `BIT0MASK` 0-vs-0xFFFFFFFF leakage is eliminated
  by construction in the shared-output masked variant, where both
  operands of any mask-style operation are uniformly random shares.
- For the unmasked encoder, a residual Hamming-weight dependence
  remains on 32-bit ARM (bit 6 isolation effect) and is fully
  characterised by the `Δ_i` table above.
- For full d=1 probing-model security, use the shared-output masked
  variant.
- Real Cortex-M4 hardware measurement (ChipWhisperer + STM32F303)
  is pending; no hardware numbers are claimed in this repository.
