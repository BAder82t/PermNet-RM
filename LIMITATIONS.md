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

- **32-bit bit-6 isolation in the unmasked baseline.** On 32-bit targets,
  the 128-bit logical state is split across four physical registers.
  Message bit `m6` enters at logical position 32 (bit 0 of the physical
  word `lo1`) with no other message bit sharing that word, so its Hamming
  weight doubles through the early butterfly stages without mixing. In
  ELMO on Cortex-M0 this word's signal reaches approximately 84% of the
  peak single-bit signal of the `BIT0MASK` baseline. Paper §5.5 covers
  this; the README surfaces the same caveat. The masked d=1 composition
  is the intended mitigation; empirical confirmation is pending as above.

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
