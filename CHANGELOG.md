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

## [Unreleased] — Masked d=1 ELMO harness + M4 simulator research

### Added
- `elmo/elmo_masked_d1.c` — Thumb harness for the Boolean-masked d=1
  composition on ARM Cortex-M0. For each of 256 messages, draws a single
  pseudo-random share from a fixed-seed linear congruential PRNG,
  encodes both shares through the baseline PermNet-RM(1,7) encoder, and
  XORs the codewords under one ELMO trigger pair.
- `elmo/Makefile` extended with `elmo_masked_d1.o`, `masked_d1.elf`,
  `masked_d1.bin`, and a `run-masked` target; `make run` now runs ELMO
  on all three encoders.
- `elmo/run_table5.sh` extended to produce three comparison blocks in
  `table5.txt`:
  (1) PermNet-RM vs BIT0MASK (the paper's Table 5);
  (2) masked d=1 vs BIT0MASK (expected amplitude reduction under a
      fresh share);
  (3) masked d=1 vs unmasked PermNet-RM (residual Cortex-M0 leakage).
  Each comparison saves plots into its own subdirectory under
  `elmo/plots/`.

### Changed
- `elmo/analyze_traces.py` now accepts two optional label arguments so
  the non-default pairs are printed with the correct encoder names.
- `elmo/README.md` updated to cover the masked harness and to document
  why there is no Cortex-M4 leakage simulator available (ELMO / GILES /
  Rosita are all M0; Rosita paper explicitly notes ELMO* is not
  suitable for M4; no maintained calibrated M4 simulator as of this
  writing).

### Notes
- The masked-d=1 ELMO measurement with one fresh share per message is a
  light-weight evaluation, not a TVLA-grade fixed-vs-random sweep with
  many trials per message. Adding that is straightforward but is not
  wired up here; the current harness is sufficient to show whether
  masking breaks per-cycle correlation with `m` in the ELMO HW model.
- The LCG inside the harness is non-cryptographic; its only job is to
  make per-trace shares independent of the message for the simulation.
  Production code must use a cryptographic RNG (see
  `source/permnet_rm17_masked_d1.c`).
- No Cortex-M4 measurements have been performed; that is still
  Part 4.3 and depends on physical hardware.

## [Unreleased] — Phase 4.2 ELMO reproducibility pack

### Added
- `elmo/README.md` — standalone reproduction guide: prerequisites, ELMO
  upstream and pinned commit (`sca-research/ELMO` @ `7c4e293`), determinism
  argument (identical binary + identical `coeffs.txt` + fixed
  `randdata100000.txt` → bit-identical traces, so raw traces do not need
  to be checked in), file map, and scope (Cortex-M0 simulated, Cortex-M4
  hardware still pending).
- `elmo/run_table5.sh` — one-command reproduction wrapper. Records a
  full toolchain snapshot into `versions.txt` (arm-none-eabi-gcc, make,
  python3, numpy, matplotlib, host uname, ELMO git commit + dirty
  state), rebuilds both Thumb binaries, runs ELMO on each, pipes the
  analysis output into `table5.txt`, writes plots to `elmo/plots/`, and
  fails fast on missing prerequisites.

### Notes
- No changes to `elmo/elmo_permnet.c`, `elmo/elmo_bitmask.c`,
  `elmo/Makefile`, or `elmo/analyze_traces.py`. The Table 5 numbers are
  whatever the existing pipeline produces under your local toolchain;
  reproducibility here means "pin the inputs and capture the versions"
  rather than "regenerate cached traces from a checked-in seed".

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


