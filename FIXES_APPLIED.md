# FIXES_APPLIED

Map from the Phase 1 review prompt to the changes that have landed in this
repo. Phases 2–6 (interleaved encoder, masked d=1, ELMO rerun, paper edits,
Cortex-M4 hardware, tightened Theorem 4.2 proof) are tracked separately and
have not been applied yet.

## Part 1 — Factual fixes

### 1.1 YODO citation (Lai et al., ePrint 2025/2162)

- **Before (README):** "Lai et al. (ePrint 2025/2162, YODO) show compilers
  reintroduce data-dependent branches from the idiom at `-O2` and above."
- **After (README):** Rewritten to reflect what YODO actually demonstrates —
  ciphertext-independent, passive single-trace attacks that exploit timing
  leakages in sparse-vector processing (`gf_carryless_mul`, `find_peaks`,
  Karatsuba base cases, key re-sampling). Text now states explicitly that
  YODO does not attack the RM encoder directly, but motivates constant-time
  work across the HQC codebase.
- **Paper preprint:** The paper's Abstract and §1.3 already describe
  YODO accurately ("ciphertext-independent timing leakages from sparse-vector
  processing"). No change required there.
- **Website copy:** Not yet updated. The website repo sits at
  `/Users/baderissaei/dev/Vaultbytes/Web/...` and will be handled separately
  per Phase-1 scoping.

### 1.2 Chen et al. citation (ePrint 2026/014)

- **Paper preprint (§1.4, third "Why existing countermeasures do not fix this" paragraph):** Already describes Chen et al. correctly as a
  throughput optimisation for HQC polynomial multiplication using additive
  FFT + CRT, not an encoder countermeasure. No change required.
- **README:** Does not cite Chen et al. No change required.

### 1.3 "Provably constant-time"

- **Before (README, line 3):** "A provably constant-time Reed-Muller encoder
  for HQC via GF(2) zeta-transform butterfly decomposition."
- **After (README, line 3):** "A branch-free, fixed-topology Reed-Muller
  encoder for HQC, built from a GF(2) zeta-transform butterfly decomposition."
- **Added (README, "What is (and is not) demonstrated"):** Explicit section
  replacing the "Key results" bullet list. Separates what is measured on
  x86-64 from what is simulated on Cortex-M0 from what is not measured at all
  (Cortex-M4 hardware). Links to `LIMITATIONS.md`.

### 1.4 "We discovered" overclaiming

- **Before (README):** Not present directly in this README, but the claim
  appears on the website.
- **After (README):** Added a sentence acknowledging the classical origin of
  the zeta-transform / Yates-algorithm equivalence so the README itself cannot
  be read as a discovery claim. Website copy change deferred to Phase 1b.

### 1.5 Leftover "Also add your ORCID" prompt

- **Status:** Not present in the current README (grep returned no matches).
  Possibly already cleaned up in a prior commit. ORCID now added directly.

### 1.6 "Zero timing spread" qualifier

- **Before (README):** "Zero timing spread across all 256 input bytes at
  `-O3`". No platform qualifier.
- **After (README):** Stated as "Zero cycle-count timing spread on x86-64 at
  `-O3`" and paired with the explicit Cortex-M0 / Cortex-M4 caveat and a
  pointer to `LIMITATIONS.md`.

## Part 6 — Repository hygiene

### 6.1 CHANGELOG.md

- Created at repo root. First entry documents all Phase 1 changes.

### 6.2 LIMITATIONS.md

- Created at repo root. Structured as four sections:
  1. What is demonstrated (x86-64 branch-free + cycle-count spread,
     exhaustive correctness)
  2. What is simulated only (ARM Cortex-M0 via ELMO)
  3. What is not yet measured (Cortex-M4 hardware, interleaved variant,
     masked d=1, non-Intel platforms)
  4. Known residual leakage (32-bit bit-6 isolation at ~84% of BIT0MASK
     peak signal)

### 6.3 CI

- Created `.github/workflows/ci.yml`.
- Matrix: `O0`, `O1`, `O2`, `O3`, `Os`, `Ofast`.
- For each level:
  - Compile `source/permnet_rm17.c`, run exhaustive correctness test.
  - Compile `source/permnet_rm18.c`, run exhaustive correctness test.
  - Extract the encoder body from the object file with
    `objdump -d` + an `awk` range filter from the function label to the
    next blank line (portable across GNU binutils and LLVM objdump).
  - Fail the job if the extracted body contains any conditional-jump
    mnemonic (`je`, `jne`, `jg`, `jge`, `jl`, `jle`, `ja`, `jae`, `jb`,
    `jbe`, `jc`, `jnc`, `jz`, `jnz`, `js`, `jns`, `jo`, `jno`, `jp`,
    `jnp`, `jcxz`, `jecxz`, `jrcxz`, `jpe`, `jpo`, `loop`, `loope`,
    `loopne`, `loopz`, `loopnz`). Unconditional `jmp` is allowed.
  - Uploads the extracted disassembly as a build artifact for inspection.

### 6.4 Jeon et al. citation verification

- **Jeon et al. (ePrint 2026/071) abstract, verified via WebSearch on
  2026-04-19:** "Using a total of 5,000 power traces for profiling and
  evaluation, the research recovers the full 128-bit encapsulation message
  from a single decapsulation trace with up to 96.9% success." Primary
  target: STM32F303 (Cortex-M4).
- **Action taken:** README aligned to the abstract (96.9% success, 5,000
  total profiling+evaluation traces, STM32F303). The paper preprint's §1.3
  uses different numbers (98.9% / 20 profiling traces on STM32F303 and
  99.5% / 60 profiling traces on STM32F415) — these look like *body*
  sub-results quoting specific experiments or a different metric than the
  headline success figure. The paper authors should verify §1.3 matches
  the intended reading of the abstract before re-submission.

## Part 2 — Interleaved injection (partial, with negative result)

### 2.1 First attempt: stage reordering — does not work

- **File:** `source/permnet_rm17_stage_reordered.c`.
- **Idea:** the 7 butterfly stages commute (they act on orthogonal hypercube
  axes), so running them in the order `k = 5, 6, 0, 1, 2, 3, 4` gives the
  same output. Correctness is verified exhaustively (256/256 inputs match
  the baseline byte-for-byte at all six GCC opt levels).
- **Why it does not fix the bit-6 / bit-7 isolation leakage:** every
  butterfly stage is a LEFT shift only. Stage k = 5 (`shift-by-32`) moves
  `lo0 -> lo1`, never `lo1 -> lo0`. Stage k = 6 moves `lo -> hi`, never
  `hi -> lo`. For single-hot messages with only m6 or m7 set, the isolated
  bit cannot be pulled back into a word shared with other message bits.
  The per-stage 32-bit Hamming-weight trace (`-v` flag of the test binary)
  makes this visible: on `msg = 0x40`, `HW(lo0, lo1, hi0, hi1)` is
  `(0, 1, 0, 0) -> (0, 1, 0, 0) -> (0, 1, 0, 1) -> (0, 2, 0, 2) -> ...`
  post-k5 is unchanged, post-k6 copies the isolation into `hi1`, and the
  subsequent within-word stages propagate `m6` in `lo1` and `hi1` alone.
  Same shape for `msg = 0x80`.
- **Status:** kept in the repo as a documented negative result. The file's
  top-of-file comment explains the failure mode so a future contributor does
  not rediscover the same dead end.

### 2.2 True interleaved injection — deferred

- The paper's Section 5.5 suggestion ("spread the singleton positions across
  words in a way that forces mixing earlier") requires placing message bits
  at positions other than the delta basis `{0, 1, 2, 4, 8, 16, 32, 64}`.
  Because the zeta transform is invertible and its inverse (over GF(2))
  equals itself, `T_std` is the unique 128-by-8 placement such that
  `Z * T = G`. Any other placement `T'` requires replacing the butterfly
  `Z` with a distinct linear map `B` satisfying `B * T' = G`. `B` can be
  constructed (it is not unique; the 120-dimensional complement is free)
  but it is not in general a product of fixed-axis butterfly stages, and
  designing an efficient straight-line XOR network for `B` is open work.
- **Status:** deferred. Phase 3 (Boolean masking d = 1) provides an
  alternative mitigation for the same leakage path that is correct by
  linearity and straightforward to implement.

## Part 3 — Boolean masking, d = 1

- **File:** `source/permnet_rm17_masked_d1.c`.
- **Implementation:** takes two 1-byte Boolean shares `s0, s1` with
  `m = s0 XOR s1`; encodes each share independently with the baseline
  PermNet-RM(1,7) encoder; XORs the codewords. A compiler barrier
  (`__asm__ volatile ("" ::: "memory")`) sits between the two share
  encodings to defeat fusion.
- **Correctness:** exhaustive test across all 256 × 256 = 65,536
  `(s0, s1)` pairs passes at `-O0, -O1, -O2, -O3, -Os, -Ofast`. Correctness
  follows directly from the linearity of the baseline encoder over GF(2)
  (`E(0) = 0` and `E(a XOR b) = E(a) XOR E(b)`); the exhaustive test is a
  sanity check on that linearity.
- **Benchmark:** `source/permnet_rm17_bench.c` extended with a fourth
  encoder entry (`masked-d1`). The benchmark's share is a deterministic
  counter so the run is reproducible; production code MUST use a
  cryptographic RNG per call. The per-input timing-spread harness is
  unchanged and applies to the masked wrapper the same way.
- **Side-channel status:** correctness is proven; abstract d = 1 probing
  model security follows by standard masking arguments when `s0` is drawn
  uniformly at random. No ELMO / hardware power measurement has been
  performed in this commit; those belong to Phase 4.
- **CI:** build + run the exhaustive 65,536-pair test at every GCC
  optimisation level; also grep the masked encoder body for conditional-
  jump mnemonics.

## Cortex-M4 simulator alternatives — researched and ruled out

Before doing more work on Part 4.3, we checked the public state of M4
leakage simulators so the roadmap reflects reality:

- **ELMO** (`sca-research/ELMO`) — Cortex-M0 only by design; the emulator
  is a custom Thumbulator and the power model is calibrated for the M0.
- **GILES** (`sca-research/GILES`) — integrates ELMO's power model into a
  leakage-assessment tool. Still Cortex-M0; latest release June 2019; not
  actively maintained.
- **ROSITA / ELMO***` — explicitly stated in the ROSITA paper that the
  underlying model "may not be suitable for more advanced processors like
  the Cortex-M4." Intended as a leakage-transformation pass on ELMO, not
  a new simulator.
- Several Cortex-M emulators exist (`zmu`, `armv6m-sim`, `memu`,
  `usim`, etc.) but without a calibrated M4 power model they do not
  substitute for ELMO.

Conclusion: no maintained, publicly available, Cortex-M4-calibrated
leakage simulator exists at the time of writing. The only credible route
to an M4 claim remains real hardware (ChipWhisperer + STM32F303 /
STM32F415), i.e. Part 4.3 as originally scoped.

## Part 4.1 / 4.2 — ELMO reproducibility

- **Added `elmo/README.md`** — standalone reproduction guide. Lists
  prerequisites (`arm-none-eabi-gcc`, `make`, `python3`, `numpy`,
  optional `matplotlib`), explains where to clone ELMO (upstream
  `sca-research/ELMO` pinned at commit `7c4e293`), documents the
  determinism argument (same binary + same `coeffs.txt` +
  `randdata100000.txt` → bit-identical ELMO traces), and walks through
  the output artifacts.
- **Added `elmo/run_table5.sh`** — one-command wrapper that:
  1. Records a full toolchain snapshot (`arm-none-eabi-gcc --version`,
     `make --version`, `python3` / `numpy` / `matplotlib` versions, host
     `uname`, and the current git commit + dirty state of `../elmo_tool/`)
     into `versions.txt`.
  2. Runs `make clean && make` to rebuild `permnet.bin`, `bitmask.bin`,
     and the new `masked_d1.bin`.
  3. Runs `make run` (ELMO over all three binaries, producing
     `output_{permnet,bitmask,masked}/output/traces/`).
  4. Runs `analyze_traces.py` three times with explicit labels:
     (a) PermNet-RM vs BIT0MASK — the paper's Table 5.
     (b) masked d=1 vs BIT0MASK — attack-relevant signal reduction under
         a fresh share.
     (c) masked d=1 vs unmasked PermNet-RM — does masking further reduce
         residual Cortex-M0 leakage on top of the butterfly encoder.
     Writes all three comparison blocks into `table5.txt` and separates
     the plots into `plots/permnet_vs_bitmask/`, `plots/masked_vs_bitmask/`,
     `plots/masked_vs_permnet/`.
  5. Fails fast if `arm-none-eabi-gcc`, `make`, `python3`, `../elmo_tool/`,
     or the built ELMO binary are missing.
- **Added `elmo/elmo_masked_d1.c`** — Thumb harness for the masked d=1
  composition. For each of 256 messages, draws one pseudo-random share
  `s0` from a fixed-seed linear congruential PRNG (the LCG is
  non-cryptographic and is only there to break per-message correlation
  for the ELMO measurement; production must use a cryptographic RNG).
  Each masked encoding is wrapped in one trigger pair, so ELMO produces
  256 per-input traces in the same shape as the other two harnesses.
  `elmo/Makefile` extended with the `masked_d1` target and the
  `run-masked` phony target; `.PHONY` and `clean` updated accordingly.
- **`elmo/analyze_traces.py`** — accepts two optional label arguments so
  the three comparisons above each print with the correct pair of
  encoder names.
- **Cortex-M0 vs Cortex-M4 caveat** (Part 4.1) already surfaced in
  `README.md` under "What is (and is not) demonstrated", in
  `LIMITATIONS.md`, and now in `elmo/README.md`'s "Scope" section.
- **What is *not* in this commit:** Part 4.3 (real Cortex-M4 hardware
  measurements). Physical-device runs are called out as the next
  highest-priority step in `LIMITATIONS.md`. No hardware numbers are
  fabricated anywhere; the ELMO reproduction is labelled as simulation
  throughout.

## Not yet done in this phase
- **Part 4.3** — Real Cortex-M4 hardware measurements (pending
  ChipWhisperer + STM32F303/F415 access).
- **Part 5** — Tightened Theorem 4.2 proof in the paper.
- **Part 6.4** — DONE. Jeon abstract verified (96.9% / 5,000 total traces /
  STM32F303); README realigned. Paper §1.3 body numbers (98.9% / 20
  profiling) remain for the paper authors to reconcile internally.
- **Website copy (`vaultbytes.com/research-permnet-rm`)** — Phase 1b.

## Honesty notes

No empirical numbers were changed or invented in this pass. The bit-6 / 84%
figure in README and LIMITATIONS comes directly from the paper's §5.5. The
"6 cycles at `-O3`" figure comes from the paper's x86-64 benchmark section.
The ELMO-based measurements are labelled as simulation, not hardware.
