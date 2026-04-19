# Limitations

Scope note for readers, reviewers, and downstream integrators.

## What is demonstrated

- **Binary-level branch-free on x86-64.** The PermNet-RM encoder, the
  stage-reordered variant, and the masked d=1 composition all compile to
  straight-line code (no `Jcc` conditional-jump mnemonics in the encoder
  body) under `gcc` at `-O0`, `-O1`, `-O2`, `-O3`, `-Os`, and `-Ofast`.
  Verified by disassembly grep; enforced in CI.
- **Zero cycle-count spread on x86-64 at `-O3`.** All 256 RM(1,7) inputs
  take exactly 6 cycles in our TSC measurement on an Intel Core Ultra 9
  275HX. This is a timing (not power) observation on a specific platform.
- **Exhaustive algebraic correctness.**
  - Baseline RM(1,7) encoder matches the HQC reference over all 256 inputs.
  - RM(1,8) encoder matches the reference over all 512 inputs.
  - Masked d=1 composition matches the baseline over all 65,536 `(s0, s1)`
    share pairs (direct test of GF(2) linearity).

## What is simulated only

- **ARM Cortex-M0 via ELMO.** ELMO is a statistical power simulator
  (McCann et al., 2017) calibrated for the Cortex-M0. Paper Table 5 results
  come from ELMO-generated traces. ELMO is not a silicon measurement; it
  does not capture every micro-architectural side effect of a physical
  device.

## What is not yet measured

- **Real Cortex-M4 hardware.** The Jeon et al. attack (ePrint 2026/071)
  targets an STM32F303 (Cortex-M4), with a noisier STM32F415 secondary
  target. No ChipWhisperer or equivalent measurements on M4 silicon have
  been run. This is the single most important next step.
- **Power-side-channel evaluation of the masked d=1 composition.** An
  ELMO harness (`elmo/elmo_masked_d1.c`) has been added and is wired into
  `elmo/run_table5.sh`, but the per-message fresh-share approach is a
  light-weight evaluation, not a full TVLA sweep with many trials per
  message. The Cortex-M4 hardware measurement that would matter most for
  the Jeon attack is still outstanding.
- **Performance on platforms other than x86-64 Intel Core Ultra 9 275HX.**
  Other microarchitectures, compilers, and compiler versions may produce
  different cycle-count spreads, even if branch-free.

## Compiler-introduced `neg` on isolated message bits (unmasked encoder only)

- **What.** GCC at `-O1` and above constant-folds the unmasked
  encoder's full butterfly on any register that starts with a single
  1-bit value. On x86-64, the 64-bit `high` register starts as `m7`
  and after 6 stages equals `-m7` (all ones or all zeros); GCC emits
  a single `negq` on `m7`. On Cortex-M0 the same happens in 32-bit
  halves for `lo1` (m6) and `hi0` (m7), producing `negs r1, r1` and
  `negs r4, r4`.
- **Why it matters.** This is syntactically the BIT0MASK idiom
  (`mask = -((m >> i) & 1)`) that the Jeon attack targets — a single
  instruction producing `0x00000000` or `0xFFFFFFFF` in a register,
  with the expected 0-vs-32 (Cortex-M0) or 0-vs-64 (x86-64) Hamming-
  weight leak. The ELMO bit-6 signal of `3,779.6` in the unmasked
  encoder is essentially the same instruction's leakage as the
  BIT0MASK bit-6 signal of `3,778.4`.
- **Why the unmasked encoder still helps.** For the other six bits
  (m0..m5) the `lo0` register contains a mix that the compiler
  cannot reduce to a single `neg`, so the per-bit mean signal drops
  by 4.6× overall (paper Table 5). The residual isolated-bit
  leakage is confined to m6 and m7.
- **What closes it.** The shared-output masked d=1 variant (see
  below). Under masking, the `neg` operand is a uniformly random
  share, so its 0-vs-32 leak is uncorrelated with the message bit.
  ELMO measures the shared-output variant's bit-6 signal at `191.1`
  vs the unmasked encoder's `3,779.6`.

## Known residual leakage

- **32-bit bit-6 isolation in the unmasked baseline.** On 32-bit targets
  the 128-bit logical state is split across four physical registers;
  message bit `m6` enters at logical position 32 (bit 0 of `lo1`) with
  no other message bit sharing that word, so its Hamming weight doubles
  through the early butterfly stages without mixing. The ELMO rerun of
  2026-04-19 ([`elmo/RUN_2026-04-19.md`](elmo/RUN_2026-04-19.md)) with
  the paper's coefficient file reproduces paper Table 5 exactly:
  PermNet peak 3,779.6 ≈ paper 3,780, mean 586.9 ≈ 587, leaking cycles
  36/94 match; bit 6 reaches 84.1% of the BIT0MASK peak (paper: ≈84%).
  The residual leakage is real and unchanged by the encoder alone.
- **Empirical validation of masked d=1 (reconstructed output).** The
  Cortex-M0 ELMO run of `source/permnet_rm17_masked_d1.c` reduces the
  leaking-cycle fraction from 38% (unmasked) or 68% (BIT0MASK) to 3%,
  but does **not** reduce peak amplitude because its final XOR that
  reconstructs the codeword is unmasked by construction. See
  `elmo/RUN_2026-04-19.md`.
- **Empirical validation of masked d=1 (shared output).**
  `source/permnet_rm17_masked_d1_shared_output.c` returns
  `(cw_share0, cw_share1)` with `cw_share0 XOR cw_share1 = E(m)` and
  does no unmask XOR inside the encoder. On Cortex-M0 ELMO it cuts
  peak signal from 4,493.4 (BIT0MASK) to 405.6 (**11.1×** reduction),
  mean signal from 2,687.0 to 204.6 (13.1× reduction), and leaking-
  cycle fraction from 68% to 1.6%. Bit 6 drops from 3,779.6 to 191.1
  (**19.8×** reduction on the paper §5.5 residual). The cost is an API
  change: downstream HQC code must consume both shares until in a
  region where unmasking is safe. Real Cortex-M4 hardware remains
  pending.

## Dead ends (for the record)

- **Stage reordering alone does not fix bit-6 isolation.** An initial
  attempt (`source/permnet_rm17_stage_reordered.c`) ran the cross-word
  butterfly stages first. It is algebraically equivalent to the baseline
  (verified exhaustively) but leakage-equivalent for single-hot m6 and m7
  messages, because every butterfly stage is a left shift and cannot pull
  an isolated bit back into a shared word. The file is kept in the tree
  as a documented negative result so future contributors do not repeat
  the attempt. True interleaved injection requires non-standard placement
  and a correspondingly non-standard linear network; designing that is
  open work and is not framed as a limitation of the current code, only
  as a direction not yet taken.

## Language used in this project

- "Measured" / "observed" for empirical results.
- "Proven" only where there is a mathematical proof (e.g., the partial
  security result in Theorem 4.2, whose proof is being tightened in a
  follow-up pass).
- "Branch-free" or "constant-time at the binary level on x86-64" rather
  than the unqualified "constant-time" or "provably constant-time".
- "Reduced leakage" rather than "eliminated leakage" on 32-bit ARM.
