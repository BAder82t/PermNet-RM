# Limitations

Scope note for readers, reviewers, and downstream integrators.

## What is demonstrated

- **Binary-level branch-free on x86-64.** The PermNet-RM encoder compiles to
  straight-line code (no `Jcc` conditional-jump mnemonics in the encoder body)
  under `gcc` at `-O0`, `-O1`, `-O2`, `-O3`, `-Os`, and `-Ofast`. Verified by
  disassembly grep; enforced in CI.
- **Zero cycle-count spread on x86-64 at `-O3`.** All 256 RM(1,7) inputs take
  exactly 6 cycles in our TSC measurement on an Intel Core Ultra 9 275HX. This
  is a timing (not power) observation on a specific platform.
- **Exhaustive algebraic correctness.** The encoder produces bit-identical
  output to the HQC reference `reed_muller_encode()` across the full input
  space: 256 inputs for RM(1,7), 512 for RM(1,8).

## What is simulated only

- **ARM Cortex-M0 via ELMO.** ELMO is a statistical power simulator
  (McCann et al., 2017) calibrated for the Cortex-M0. Our Table 5 results come
  from ELMO-generated traces. ELMO is not a silicon measurement; it does not
  capture every micro-architectural side effect of a physical device.

## What is not yet measured

- **Real Cortex-M4 hardware.** The Jeon et al. attack (ePrint 2026/071) targets
  an STM32F303 (Cortex-M4), with a noisier STM32F415 secondary target. We have
  not run ChipWhisperer or equivalent measurements on M4 silicon. This is the
  single most important next step and is called out as future work.
- **Interleaved 32-bit injection variant.** Not yet implemented. An initial
  attempt (`source/permnet_rm17_stage_reordered.c`) that only reordered the
  butterfly stages was shown to be **algebraically equivalent but
  leakage-equivalent** to the baseline for the exact bits the mitigation was
  supposed to fix (single-hot m6 and m7 messages), because all within-word
  and cross-word butterfly stages are left-shifts and cannot pull an isolated
  bit out of `lo1` or `hi0` back into `lo0`. True interleaved injection in
  the sense of paper Section 5.5 requires placing message bits at
  non-standard positions, which changes the linear map the butterfly
  implements; a pure stage reordering cannot achieve this. Documented as a
  negative result. Real mitigation is deferred to Phase 3 (Boolean masking
  d=1) or a bespoke linear-map redesign.
- **Boolean masking composition at d=1.** Implemented in
  `source/permnet_rm17_masked_d1.c` and exhaustively verified for all
  65,536 (share0, share1) pairs; x86-64 throughput and per-input timing
  spread reported by `source/permnet_rm17_bench.c`. Power-side-channel
  validation (ELMO on Cortex-M0, or real hardware on Cortex-M4) has not
  been performed. Until that measurement exists, the security claim for
  this construction is confined to the abstract d = 1 probing model.
- **Performance on platforms other than x86-64 Intel Core Ultra 9 275HX.**
  Other microarchitectures, compilers, and compiler versions may produce
  different cycle-count spreads, even if branch-free.

## Known residual leakage

- **32-bit bit-6 isolation.** On 32-bit targets where the 128-bit logical state
  is split across multiple physical registers, the initial placement of message
  bit 6 (at logical position 64) lands alone in a 32-bit word. In ELMO
  simulation on Cortex-M0, this word's Hamming weight reaches approximately
  84% of the peak single-bit signal amplitude of the `BIT0MASK` baseline. The
  remaining message bits are significantly attenuated but not eliminated.
  Paper §5.5 discusses this in detail; the README surfaces the same caveat.

## Language used in this project

- "Measured" / "observed" for empirical results.
- "Proven" only where there is a mathematical proof (e.g., the partial security
  result in Theorem 4.2, whose proof is being tightened in a follow-up pass).
- "Branch-free" or "constant-time at the binary level on x86-64" rather than
  the unqualified "constant-time" or "provably constant-time".
- "Reduced leakage" rather than "eliminated leakage" on 32-bit ARM.
