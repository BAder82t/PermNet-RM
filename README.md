# PermNet-RM

A provably constant-time Reed-Muller encoder for HQC via GF(2) zeta-transform butterfly decomposition.

## What this is

PermNet-RM is a drop-in replacement for `reed_muller_encode()` in the HQC reference implementation. It eliminates the `BIT0MASK` idiom (`mask = -((uint64_t)((m >> i) & 1))`) that two recent attacks exploit:

- **Jeon et al. (ePrint 2026/071)** recover individual message bits with 96.9% accuracy from a single power trace.
- **Lai et al. (ePrint 2025/2162, YODO)** show compilers reintroduce data-dependent branches from the idiom at `-O2` and above.

PermNet-RM replaces the conditional generator-row accumulation with a fixed-topology butterfly network that computes the GF(2) zeta transform. Message bits enter as initial register state at fixed positions; every subsequent operation is unconditional.

## Key results

- **Zero timing spread** across all 256 input bytes at `-O3` (every input takes exactly 6 cycles)
- **Zero conditional branches** in compiled output across all six GCC optimisation levels (`-O0` through `-Ofast`)
- **21-cycle overhead** per full HQC-128 encapsulation (~7 ns at 3 GHz) — unmeasurable in practice
- **Exhaustive correctness verification** over the complete input space (256 inputs for RM(1,7), 512 for RM(1,8))

## Files

| File | Description |
|------|-------------|
| `source/permnet_rm17.c` | RM(1,7) encoder for HQC-128 + exhaustive correctness test |
| `source/permnet_rm17_bench.c` | Benchmark comparing PermNet-RM, BIT0MASK, and branched encoders |
| `source/permnet_rm18.c` | RM(1,8) encoder for HQC-192/HQC-256 + exhaustive correctness test |
| `source/_enc_O3.s` | x86-64 disassembly at `-O3` (gcc 15.2.0) — zero Jcc in PermNet-RM |
| `source/_enc_O0.s` | x86-64 disassembly at `-O0` |

## Building

```bash
# Correctness test (RM(1,7))
gcc -O3 -o permnet_rm17 source/permnet_rm17.c && ./permnet_rm17

# Correctness test (RM(1,8))
gcc -O3 -o permnet_rm18 source/permnet_rm18.c && ./permnet_rm18

# Benchmark
gcc -O3 -march=native -o bench source/permnet_rm17_bench.c && ./bench

# Benchmark at -O0 (YODO scenario)
gcc -O0 -o bench_O0 source/permnet_rm17_bench.c && ./bench_O0
```

## How it works

1. **Injection**: Each message bit is placed at a fixed bit position (powers of 2) in an n-bit register.
2. **Butterfly propagation**: m stages of `reg ^= (reg & MASK) << SHIFT` where masks and shifts are compile-time constants.
3. **Output**: The final register state is the RM(1,m) codeword.

No message bit is ever compared, branched on, or used to index memory.

## License

MIT

## Author

Bader Alissaei — [VaultBytes Innovations Ltd](https://vaultbytes.com)
