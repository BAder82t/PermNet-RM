# PermNet-RM — The Unmasked Butterfly

A branch-free, fixed-topology Reed-Muller encoder for HQC, built from a GF(2) zeta-transform butterfly decomposition.

## What this is

PermNet-RM is a drop-in replacement for `reed_muller_encode()` in the HQC reference implementation. It eliminates the `BIT0MASK` idiom (`mask = -((uint64_t)((m >> i) & 1))`) that the Jeon et al. single-trace attack exploits:

- **Jeon et al. (ePrint 2026/071)** recover RM-input symbols on an STM32F303 (ARM Cortex-M4) and use Reed–Solomon post-correction to recover up to 98.9% of full 128-bit messages from a single decapsulation trace with as few as 20 profiling traces. The target of the attack is the RM encoder's `BIT0MASK` idiom, reached during the FO re-encryption step.
- **Lai et al. (ePrint 2025/2162, YODO)** demonstrate ciphertext-independent, passive single-trace attacks on HQC that exploit timing leakages in sparse-vector processing (`gf_carryless_mul`, `find_peaks`, Karatsuba base cases, key re-sampling). YODO does *not* attack the RM encoder directly, but it motivates constant-time work across the HQC code base. The RM encoder is a separate, complementary leakage source.

PermNet-RM replaces the conditional generator-row accumulation with a fixed-topology butterfly network that computes the GF(2) zeta transform. Message bits enter as initial register state at fixed positions; every subsequent operation is unconditional.

The equivalence between RM(1,m) codewords and the GF(2) zeta (Möbius) transform over the Boolean lattice is classical (Yates, 1937). The contribution here is an ABI-compatible, branch-free implementation that drops into HQC.

## What is (and is not) demonstrated

- **Binary-level branch-free on x86-64** across six GCC optimisation levels (`-O0` through `-Ofast`) — verified by disassembly grep for conditional-jump mnemonics.
- **Zero cycle-count timing spread on x86-64 at `-O3`** across all 256 RM(1,7) inputs under our TSC measurement.
- **Exhaustive correctness** over the complete input space (256 inputs for RM(1,7), 512 for RM(1,8)).
- **Reduced but not eliminated Hamming-weight leakage on 32-bit ARM** in ELMO simulation. A single 32-bit word can still retain dependence on an individual message bit (notably bit 6, which reaches ~84% of the BIT0MASK peak signal). See paper §5.5 and [LIMITATIONS.md](./LIMITATIONS.md).
- **Not yet measured:** real Cortex-M4 hardware (ChipWhisperer + STM32F303/F415), which is the Jeon attack's actual target platform. ELMO models Cortex-M0 only.

See [LIMITATIONS.md](./LIMITATIONS.md) for the full list.

## Files

| File | Description |
|------|-------------|
| `source/permnet_rm17.c` | RM(1,7) encoder for HQC-128 + exhaustive correctness test |
| `source/permnet_rm17_bench.c` | Benchmark comparing PermNet-RM, BIT0MASK, and branched encoders |
| `source/permnet_rm18.c` | RM(1,8) encoder for HQC-192/HQC-256 + exhaustive correctness test |
| `source/_enc_O3.s` | x86-64 disassembly at `-O3` (gcc 15.2.0) |
| `source/_enc_O0.s` | x86-64 disassembly at `-O0` |

## Building

```bash
# Correctness test (RM(1,7))
gcc -O3 -o permnet_rm17 source/permnet_rm17.c && ./permnet_rm17

# Correctness test (RM(1,8))
gcc -O3 -o permnet_rm18 source/permnet_rm18.c && ./permnet_rm18

# Benchmark
gcc -O3 -march=native -o bench source/permnet_rm17_bench.c && ./bench

# Benchmark at -O0
gcc -O0 -o bench_O0 source/permnet_rm17_bench.c && ./bench_O0
```

## How it works

1. **Injection**: Each message bit is placed at a fixed bit position (powers of 2) in an n-bit register.
2. **Butterfly propagation**: m stages of `reg ^= (reg & MASK) << SHIFT` where masks and shifts are compile-time constants.
3. **Output**: The final register state is the RM(1,m) codeword.

No message bit is ever compared, branched on, or used to index memory.
## Paper & Citation

Preprint: [Zenodo DOI 10.5281/zenodo.19556200](https://doi.org/10.5281/zenodo.19556200)
Added simple explanation in my website : https://vaultbytes.com/research-permnet-rm

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

Also add your ORCID to the Author line:

```markdown
## Author
Bader Alissaei — [VaultBytes Innovations Ltd](https://vaultbytes.com)  
ORCID: [0009-0003-5312-383X](https://orcid.org/0009-0003-5312-383X)
```


## License

MIT

## Author

Bader Alissaei — [VaultBytes Innovations Ltd](https://vaultbytes.com) — ORCID: [0009-0003-5312-383X](https://orcid.org/0009-0003-5312-383X)
