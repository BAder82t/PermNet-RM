# Changelog

All notable changes to PermNet-RM are documented here.

## [Unreleased] — Phase 1 honesty and hygiene pass

### Fixed
- **README:** YODO / Lai et al. (ePrint 2025/2162) citation rewritten. The previous
  text claimed YODO shows "compilers reintroduce data-dependent branches from the
  `BIT0MASK` idiom at `-O2` and above." This is not what Lai et al. demonstrate.
  YODO targets ciphertext-independent timing leakages in sparse-vector processing
  (`gf_carryless_mul`, `find_peaks`, Karatsuba base cases, key re-sampling); it
  motivates constant-time work across HQC but does not directly attack the RM
  encoder. Text now reflects the paper's actual claims.
- **README:** Dropped "provably constant-time" framing. Replaced with precise
  language covering what is demonstrated (binary-level branch-free on x86-64,
  zero cycle-count spread at `-O3`) versus what is only partially achieved
  (reduced-but-not-eliminated ARM Cortex-M0 ELMO leakage, with bit 6 still at
  ~84% of the `BIT0MASK` peak signal).
- **README:** "Zero timing spread" now qualified with "on x86-64 at `-O3`" and
  paired with an explicit note that 32-bit ARM retains residual Hamming-weight
  leakage.
- **README:** "We discovered that Reed-Muller encoding is identical to a GF(2)
  zeta transform" softened. The equivalence is classical (Yates, 1937); the
  contribution is a branch-free, ABI-compatible implementation.
- **README:** Added Cortex-M0 vs Cortex-M4 caveat. ELMO models M0; the Jeon
  attack targets an STM32F303 (M4). M4 hardware measurement remains future work.
- **README:** ORCID added to the Author line (0009-0003-5312-383X).

### Added
- `LIMITATIONS.md` — standalone summary of what is demonstrated, what is
  simulated only, what is not yet measured, and known residual leakage.
- `CHANGELOG.md` — this file.
- `FIXES_APPLIED.md` — mapping from the Phase 1 fix list to commits.
- `.github/workflows/ci.yml` — compiles the encoders at `-O0`, `-O1`, `-O2`,
  `-O3`, `-Os`, `-Ofast`; runs exhaustive correctness tests; greps the encoder
  disassembly for conditional-jump mnemonics and fails if any are found.

### Notes
- No changes to the baseline encoder in `source/permnet_rm17.c` or
  `source/permnet_rm17_bench.c`. Phase 3 (Boolean masking d=1) planned next.

## [Unreleased] — Phase 2 exploratory variant (negative result)

### Added
- `source/permnet_rm17_stage_reordered.c` — a PermNet-RM(1,7) encoder that
  runs the butterfly stages in the order `k = 5, 6, 0, 1, 2, 3, 4` instead of
  `0..6`. Algebraically equivalent to the baseline (butterfly-stage
  commutativity over disjoint hypercube axes). Verified exhaustively against
  the baseline for all 256 messages at all six GCC optimisation levels.

### Why this is in the repo as a negative result
- The stage-reordered variant is **not** the interleaved-injection mitigation
  described in paper §5.5, and it does not reduce the bit-6 / bit-7
  isolation leakage. Each butterfly stage is a left shift only, so running
  the cross-word stages first cannot pull an isolated `m6` out of `lo1` (or
  `m7` out of `hi0`) back into `lo0`. The per-stage 32-bit Hamming-weight
  trace (`./permnet_rm17_stage_reordered -v`) makes the asymmetry visible on
  single-hot inputs.
- True interleaved injection in the sense of the paper requires placing
  message bits at non-standard positions, which changes the linear map the
  butterfly implements; pure stage reordering cannot achieve that.
- Kept in the tree so future contributors do not rediscover the dead end.

### CI
- `.github/workflows/ci.yml` extended to build and run the stage-reordered
  variant, and to grep its encoder body for conditional jumps across all
  six GCC optimisation levels.

### Not yet delivered
- True interleaved injection (non-standard placement + corresponding linear
  post-network) — open work.
- ELMO rerun — deferred until a variant that actually changes the bit-6
  trajectory exists.

## [Unreleased] — Phase 3 Boolean masking, d = 1

### Added
- `source/permnet_rm17_masked_d1.c` — Boolean-masked PermNet-RM(1,7)
  composition at d = 1. Takes two 1-byte Boolean shares `s0, s1` with
  `m = s0 XOR s1`, encodes each share through the baseline encoder, and
  XORs the resulting codewords. Correct by linearity of the encoder over
  GF(2); exhaustively verified across all 65,536 `(s0, s1)` share pairs at
  every GCC optimisation level (`O0..Ofast`). A compiler barrier
  (`__asm__ volatile ("" ::: "memory")`) separates the two share encodings
  to prevent inter-share fusion by the optimiser.
- `source/permnet_rm17_bench.c` extended with:
  - A fourth encoder entry (`masked-d1`) in both the throughput and
    per-input timing distribution tables.
  - A spot-check masked-vs-baseline correctness gate in `verify_all_three()`
    (4 random share splits per message).
- CI: compiles and runs the exhaustive masked correctness harness, and
  grep-checks the masked encoder body for conditional-jump mnemonics at
  every optimisation level.

### Notes and honesty
- The d = 1 probing-model argument is about abstract leakage, not a claim
  about any specific microprocessor. Empirical validation on ELMO
  (Cortex-M0) or ChipWhisperer (Cortex-M4) has not been performed in this
  commit.
- The deterministic counter-based share used by the benchmark wrapper is
  there solely for reproducible throughput measurement. Real deployments
  must draw `s0` from a cryptographic RNG per call and never reuse a share.
- The masked composition roughly doubles encoder cost. x86-64 throughput
  figures are printed by the bench harness; cross-platform numbers are
  out of scope for this commit.


