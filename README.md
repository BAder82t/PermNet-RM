# PermNet-RM — The Unmasked Butterfly

A branch-free, fixed-topology Reed-Muller encoder for HQC, built from a GF(2) zeta-transform butterfly decomposition.

## What this is

PermNet-RM is a drop-in replacement for `reed_muller_encode()` in the HQC reference implementation. It eliminates the `BIT0MASK` idiom (`mask = -((uint64_t)((m >> i) & 1))`) that the Jeon et al. single-trace attack exploits:

- **Jeon et al. (ePrint 2026/071)** recover RM-input symbols on an STM32F303 (ARM Cortex-M4) and use Reed–Solomon post-correction to recover up to 98.9% of full 128-bit messages from a single decapsulation trace with as few as 20 profiling traces. The target of the attack is the RM encoder's `BIT0MASK` idiom, reached during the FO re-encryption step.
- **Lai et al. (ePrint 2025/2162, YODO)** demonstrate ciphertext-independent, passive single-trace attacks on HQC that exploit timing leakages in sparse-vector processing (`gf_carryless_mul`, `find_peaks`, Karatsuba base cases, key re-sampling). YODO does *not* attack the RM encoder directly, but it motivates constant-time work across the HQC code base. The RM encoder is a separate, complementary leakage source.

PermNet-RM replaces the conditional generator-row accumulation with a fixed-topology butterfly network that computes the GF(2) zeta transform. Message bits enter as initial register state at fixed positions; every subsequent operation is unconditional.

The equivalence between RM(1,m) codewords and the GF(2) zeta (Möbius) transform over the Boolean lattice is classical (Yates, 1937). The contribution here is an ABI-compatible, branch-free implementation that drops into HQC.

## What is (and is not) demonstrated

- **Binary-level branch-free on x86-64** across six GCC optimisation levels (`-O0` through `-Ofast`) — verified by disassembly grep for conditional-jump mnemonics; enforced in CI.
- **Zero cycle-count timing spread on x86-64 at `-O3`** across all 256 RM(1,7) inputs under our TSC measurement.
- **Exhaustive correctness** over the complete input space (256 inputs for RM(1,7), 512 for RM(1,8), and 65,536 `(share0, share1)` pairs for the masked d=1 composition).
- **Reduced but not eliminated Hamming-weight leakage on 32-bit ARM** in ELMO simulation. A single 32-bit word can still retain dependence on an individual message bit (notably bit 6, which reaches 84.1% of the BIT0MASK peak signal — reproduced below). See paper §5.5 and [LIMITATIONS.md](./LIMITATIONS.md).
- **Not yet measured:** real Cortex-M4 hardware (ChipWhisperer + STM32F303/F415), which is the Jeon attack's actual target platform. ELMO models Cortex-M0 only; no maintained public Cortex-M4 leakage simulator exists (see [elmo/README.md](./elmo/README.md)).

See [LIMITATIONS.md](./LIMITATIONS.md) for the full list.

## Recent findings (2026-04-19)

### Paper Table 5 is reproducible

Running the one-command [`elmo/run_table5.sh`](./elmo/run_table5.sh) with the pinned ELMO commit (`sca-research/ELMO` @ `7c4e293`) and `coeffs_M3.txt` reproduces paper Table 5 to four significant figures on a current toolchain (`arm-none-eabi-gcc` 15.2.0):

| Metric | Rerun (2026-04-19) | Paper §5.5 |
|---|---:|---:|
| PermNet trace length | 94 cycles | 94 |
| PermNet max single-bit signal | 3,779.6 | 3,780 |
| PermNet mean single-bit signal | 586.9 | 587 |
| BIT0MASK trace length | 293 cycles | 293 |
| BIT0MASK max single-bit signal | 4,493.4 | 4,493 |
| BIT0MASK mean single-bit signal | 2,687.0 | 2,687 |
| Leaking cycles PermNet | 36/94 | 36/94 |
| Leaking cycles BIT0MASK | 199/293 | 199/293 |
| Bit 6 ratio to BIT0MASK peak | 84.1% | ≈84% |
| Mean reduction | 4.58× | 4.6× |

See [`elmo/RUN_2026-04-19.md`](./elmo/RUN_2026-04-19.md) for the full reproduction report.

### Masked d=1 reduces leaking surface but not peak amplitude

A Boolean-masked d=1 composition is implemented in [`source/permnet_rm17_masked_d1.c`](./source/permnet_rm17_masked_d1.c) (exhaustively verified over all 65,536 share pairs) and a matching ELMO harness in [`elmo/elmo_masked_d1.c`](./elmo/elmo_masked_d1.c). The measured effect on Cortex-M0 ELMO:

| Metric | Unmasked PermNet | Masked d=1 | BIT0MASK |
|---|---:|---:|---:|
| Peak single-bit signal | 3,779.6 | 3,794.5 | 4,493.4 |
| Leaking-cycle fraction | 38% (36/94) | **3% (7/211)** | 68% (199/293) |

Masking reduces the leaking **surface** by an order of magnitude. It does **not** reduce the peak amplitude, because the final XOR that reconstructs the codeword from its two shares is unmasked by construction and leaks the message bit on that single cycle. Proper output-side protection needs either (a) a shared-output API so downstream HQC code consumes the two halves, or (b) a TVLA-style multi-seed sweep to average away the unmask leakage. Neither is in scope here.

### Paper Theorem 4.2 as stated is not correct

Independent verification (analytical + exhaustive brute force over all 256 RM(1,7) messages) shows that the paper's Theorem 4.2 (`Corr( wt(R^(1)), a_i ) = 0` for every message bit after the first butterfly stage, under uniform remaining bits) holds only for `a_1`. For the other seven bits, the conditional-expectation gap `Δ_i = E[wt | a_i = 1] − E[wt | a_i = 0]` is:

| i | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Δ_i | **1** | 0 | **2** | **2** | **2** | **2** | **2** | **2** |

`a_0` survives unmasked at position `∅`; `a_i` for `i ≥ 2` survives at both `{i-1}` and `{0, i-1}`, giving a gap of 2. The proof's "linearity and symmetry" justification confuses GF(2)-linearity of the zeta transform with integer-linearity of the Hamming weight; the two are not the same.

Analytical derivation, a **"where it fails" walk-through**, a proof that the unmasked encoder **cannot structurally be fixed** to make the original theorem hold (the `a_0 → ∅` placement is forced by the zeta transform), and two restated theorems that are actually true are in [`PROOF_NOTES.md`](./PROOF_NOTES.md). Boolean masking at d=1 (already in `source/permnet_rm17_masked_d1.c`) is the structural fix that recovers the intended probing-security property. The brute-force verification script is [`source/verify_theorem_4_2.py`](./source/verify_theorem_4_2.py) — run `python3 source/verify_theorem_4_2.py` to regenerate the table.

**What this means for downstream users (HQC team):** the encoder C code is unchanged and correct; the Jeon-style `BIT0MASK` leakage is eliminated; the Cortex-M0 ELMO reduction (4.6× mean, 84% peak) is real and reproducible; only the paper's formal §4.2 claim needs restating. For full probing-model security, use the masked d=1 variant. See [`PROOF_NOTES.md`](./PROOF_NOTES.md) "Bottom line for the HQC team" section.

### Stage reordering does NOT fix bit-6 isolation

An exploratory [`source/permnet_rm17_stage_reordered.c`](./source/permnet_rm17_stage_reordered.c) kept in the tree as a documented negative result: the 7 butterfly stages commute (they act on orthogonal hypercube axes), so running the cross-word stages first is algebraically equivalent, but every stage is a left shift and cannot pull an isolated `m6` or `m7` back into a shared 32-bit word. True interleaved injection per paper §5.5 requires non-standard placement and a correspondingly non-standard linear network; that is open work.

## Files

| File | Description |
|------|-------------|
| `source/permnet_rm17.c` | RM(1,7) encoder for HQC-128 + exhaustive correctness test |
| `source/permnet_rm17_bench.c` | x86-64 benchmark — PermNet, BIT0MASK, branchy, masked-d1 |
| `source/permnet_rm17_masked_d1.c` | Boolean-masked d=1 composition + 65,536-pair exhaustive test |
| `source/permnet_rm17_stage_reordered.c` | Exploratory stage-reordered variant (documented negative result) |
| `source/permnet_rm18.c` | RM(1,8) encoder for HQC-192/HQC-256 + exhaustive correctness test |
| `source/verify_theorem_4_2.py` | Brute-force check of paper Theorem 4.2 |
| `source/_enc_O3.s` | x86-64 disassembly at `-O3` (gcc 15.2.0) |
| `source/_enc_O0.s` | x86-64 disassembly at `-O0` |
| `elmo/` | Thumb harnesses, Makefile, and `run_table5.sh` for the ELMO reproduction pack |
| `elmo/RUN_2026-04-19.md` | Headline numbers and paper-comparison from the last ELMO rerun |
| `LIMITATIONS.md` | Scope, simulated-vs-measured, known residual leakage |
| `PROOF_NOTES.md` | Theorem 4.2 correction with restatement and evidence |
| `CHANGELOG.md` | Phase-by-phase change log |
| `FIXES_APPLIED.md` | Mapping from review prompt items to code changes |
| `.github/workflows/ci.yml` | CI: build + exhaustive tests + disassembly grep at `-O0..-Ofast` |

## Building

```bash
# Correctness test (RM(1,7))
gcc -O3 -o permnet_rm17 source/permnet_rm17.c && ./permnet_rm17

# Correctness test (RM(1,8))
gcc -O3 -o permnet_rm18 source/permnet_rm18.c && ./permnet_rm18

# Masked d=1: exhaustive 65,536-pair check
gcc -O3 -o permnet_rm17_masked source/permnet_rm17_masked_d1.c && ./permnet_rm17_masked

# Stage-reordered variant: correctness + per-stage HW trace (`-v`)
gcc -O3 -o permnet_rm17_sr source/permnet_rm17_stage_reordered.c && ./permnet_rm17_sr

# Benchmark (x86-64 only, uses TSC via <x86intrin.h>)
gcc -O3 -march=native -o bench source/permnet_rm17_bench.c && ./bench

# Theorem 4.2 brute-force verification
python3 source/verify_theorem_4_2.py

# ELMO reproduction pack (requires ../elmo_tool/ + arm-none-eabi-gcc)
cd elmo && ./run_table5.sh
```

## How it works

1. **Injection**: Each message bit is placed at a fixed bit position (powers of 2) in an n-bit register.
2. **Butterfly propagation**: m stages of `reg ^= (reg & MASK) << SHIFT` where masks and shifts are compile-time constants.
3. **Output**: The final register state is the RM(1,m) codeword.

No message bit is ever compared, branched on, or used to index memory.

## Paper & Citation

Preprint: [Zenodo DOI 10.5281/zenodo.19556200](https://doi.org/10.5281/zenodo.19556200)

Plain-English summary: <https://vaultbytes.com/research-permnet-rm>

> The repository README is ahead of the current preprint on two points: the §5.5 numerical reproduction (consistent with the preprint) and the §4.2 theorem correction (a local proof fix). See [`PROOF_NOTES.md`](./PROOF_NOTES.md) before citing the preprint's Theorem 4.2.

**BibTeX:**
```bibtex
@misc{alissaei2026permnet,
  title     = {PermNet-RM: Eliminating Side-Channel Leakage in HQC
               Reed-Muller Encoding via the GF(2) Zeta Transform},
  author    = {Alissaei, Bader},
  year      = {2026},
  publisher = {Zenodo},
  doi       = {10.5281/zenodo.19556200},
  url       = {https://doi.org/10.5281/zenodo.19556200}
}
```

## License

MIT

## Author

Bader Alissaei — [VaultBytes Innovations Ltd](https://vaultbytes.com) — ORCID: [0009-0003-5312-383X](https://orcid.org/0009-0003-5312-383X)
