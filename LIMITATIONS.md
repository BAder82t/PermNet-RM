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

## Known residual leakage

- **32-bit bit-6 isolation in the unmasked baseline.** On 32-bit targets
  the 128-bit logical state is split across four physical registers;
  message bit `m6` enters at logical position 32 (bit 0 of `lo1`) with
  no other message bit sharing that word, so its Hamming weight doubles
  through the butterfly stages. After the compiler-barrier fix (see
  `elmo/RUN_2026-04-19.md`), bit 6 peaks at 1,757.7 on Cortex-M0 ELMO
  (2.15× below the `BIT0MASK` bit-6 peak of 3,778.4 and 50% below the
  pre-fix PermNet bit-6 peak). Bit 7 peaks at 221.2 (17× below
  `BIT0MASK`). The shared-output masked d=1 variant drives bit 6 down
  further to 191.1.
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
