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
- No algorithmic changes in this pass. Encoder code is unchanged.
- Phase 2 (interleaved 32-bit injection variant) and Phase 3 (Boolean masking
  composition at d=1) are planned next.
