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
- **Paper (`real text.md`):** The paper's Abstract and §1.3 already describe
  YODO accurately ("ciphertext-independent timing leakages from sparse-vector
  processing"). No change required there.
- **Website copy:** Not yet updated. The website repo sits at
  `/Users/baderissaei/dev/Vaultbytes/Web/...` and will be handled separately
  per Phase-1 scoping.

### 1.2 Chen et al. citation (ePrint 2026/014)

- **Paper (`real text.md`, §1.4 / third "Why existing countermeasures do not
  fix this" paragraph):** Already describes Chen et al. correctly as a
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

- **Current README numbers:** "96.9% accuracy from a single power trace" and
  "STM32F303 (ARM Cortex-M4) target" wording.
- **Paper (`real text.md`, §1.3):** Uses "98.9% full 128-bit message recovery
  from a single decapsulation trace with as few as 20 profiling traces" and
  "99.5% with 60 profiling traces on STM32F415".
- **Mismatch:** The README and paper disagree on the headline percentage
  (96.9% vs 98.9%). I have not re-read the Jeon abstract in this pass to
  pick the correct one; fetching the abstract is flagged as a follow-up item
  below. Pending that, the README was updated to the paper's numbers
  (98.9% / 20 profiling traces) so the two documents agree internally. Please
  verify against the ePrint abstract before publishing.

## Not yet done in this phase

- **Part 2** — Interleaved 32-bit injection encoder + exhaustive correctness +
  ELMO re-run.
- **Part 3** — Boolean masking composition at d=1 + benchmark + ELMO.
- **Part 4.1/4.2** — ELMO reproducibility artifacts (standalone
  `elmo/README.md` with exact toolchain and a single command to reproduce
  Table 5).
- **Part 4.3** — Real Cortex-M4 hardware measurements (pending
  ChipWhisperer + STM32F303/F415 access).
- **Part 5** — Tightened Theorem 4.2 proof in the paper.
- **Part 6.4 verification** — Re-read the Jeon ePrint abstract and reconcile
  the 96.9% vs 98.9% figure across README and paper. Flagged above.
- **Website copy (`vaultbytes.com/research-permnet-rm`)** — Phase 1b.

## Honesty notes

No empirical numbers were changed or invented in this pass. The bit-6 / 84%
figure in README and LIMITATIONS comes directly from the paper's §5.5. The
"6 cycles at `-O3`" figure comes from the paper's x86-64 benchmark section.
The ELMO-based measurements are labelled as simulation, not hardware.
