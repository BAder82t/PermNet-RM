# ELMO reproducibility pack for PermNet-RM

This directory contains everything needed to reproduce the paper's Table 5
(per-bit signal amplitude and leaking-cycle counts for PermNet-RM vs the
`BIT0MASK` baseline on ARM Cortex-M0 under the ELMO statistical power
simulator).

## One-command reproduction

```bash
cd elmo/
./run_table5.sh
```

`run_table5.sh` is idempotent. It:

1. Writes [`versions.txt`](versions.txt) with the exact versions of every tool
   it invokes (OS, `arm-none-eabi-gcc`, `make`, the ELMO binary, and the git
   commit of the `../elmo_tool/` checkout).
2. Rebuilds `permnet.bin`, `bitmask.bin`, and `masked_d1.bin` from source
   (`make clean && make`).
3. Runs ELMO on all three binaries (`make run`). This creates
   `output_permnet/output/traces/`, `output_bitmask/output/traces/`, and
   `output_masked/output/traces/`, each holding 256 per-input trace files.
4. Runs [`analyze_traces.py`](analyze_traces.py) three times: PermNet-RM vs
   BIT0MASK (the paper's Table 5), masked-d1 vs BIT0MASK (how much the
   abstract-d=1 mask reduces the attack-relevant amplitude), and masked-d1
   vs unmasked PermNet-RM (does the mask further reduce residual leakage on
   Cortex-M0).
5. Writes the three summary blocks into [`table5.txt`](table5.txt) and
   separate plots subdirectories (`plots/permnet_vs_bitmask/`,
   `plots/masked_vs_bitmask/`, `plots/masked_vs_permnet/`).

If any step fails, the script stops and prints the failing command. There are
no hidden defaults — every flag that goes into the build or the analysis is
either in this file, `Makefile`, or `analyze_traces.py`.

## Prerequisites

| Tool | Purpose | Tested version |
|------|---------|----------------|
| `arm-none-eabi-gcc` | Cortex-M0 Thumb cross-compiler | any modern release (`gcc -v` recorded in `versions.txt` at run time) |
| `make` | build orchestration | GNU Make 3.81+ |
| `python3` | analysis script | 3.8+ |
| `numpy` | analysis | any recent |
| `matplotlib` | plots (optional; skipped if missing) | any recent |
| ELMO | power simulator | see below |

### ELMO

The ELMO simulator is not vendored in this repository. Clone it separately
and place the repository at `../elmo_tool/` relative to this `elmo/`
directory.

- Upstream: <https://github.com/sca-research/ELMO>
- The configuration used to produce paper Table 5 is pinned at commit
  `7c4e293` of `sca-research/ELMO` (origin remote set to
  `https://github.com/sca-research/ELMO.git`). Later commits may work but
  have not been validated against our traces.
- `run_table5.sh` records the exact ELMO commit seen on your machine in
  `versions.txt`. If that commit differs from `7c4e293` and you observe
  numerical drift in Table 5, pin your local copy and re-run:
  ```bash
  cd ../elmo_tool/ && git checkout 7c4e293 && make && cd ../elmo/
  ```

Build the ELMO binary once, per the upstream `README.md`:

```bash
cd ../elmo_tool/
make
cd ../elmo/
```

The resulting `../elmo_tool/elmo` binary is what the Makefile's `run-permnet`
and `run-bitmask` targets invoke.

## Why this is reproducible without committing raw traces

ELMO is fully deterministic given:

1. The exact input Thumb binary (`permnet.bin`, `bitmask.bin`), which is
   reproduced byte-for-byte by `make` from the source files in this
   directory (`elmo_permnet.c`, `elmo_bitmask.c`, `vector.s`,
   `elmoasmfunctions.s`) at the compiler flags fixed in `Makefile`.
2. The ELMO `coeffs.txt` and random-data input
   (`../elmo_tool/Examples/randdata100000.txt`). The random-data file is
   fixed upstream and never regenerated.

Given identical inputs, two ELMO runs on the same machine produce
bit-identical trace files. Pinning the ELMO commit therefore pins the
traces, and we do not need to check in the traces themselves. Raw traces
are a few megabytes per run; if you want them preserved as an artifact of
a specific build, run:

```bash
tar czf traces_$(date +%Y%m%d).tar.gz output_permnet output_bitmask
```

## Files

| File | Role |
|------|------|
| `Makefile` | builds `permnet.bin` / `bitmask.bin`, runs ELMO on each, generates `.list` disassembly. |
| `elmo_permnet.c` | PermNet-RM ELMO harness: encodes all 256 inputs with start/end trigger wrapping so ELMO captures 256 per-input traces. |
| `elmo_bitmask.c` | Same harness for the BIT0MASK baseline (the classical `-((m>>i)&1)` idiom). |
| `elmo_masked_d1.c` | Harness for the Boolean-masked d=1 composition: per message, draws one pseudo-random share from a fixed-seed LCG, encodes both shares, XORs the codewords, triggers once per message. |
| `elmo.ld` | linker script: places `.text` at the reset vector ELMO expects. |
| `vector.s` | minimal Cortex-M0 vector table + reset handler for the emulator. |
| `elmoasmfunctions.s` / `.h` | ELMO's own `starttrigger()`, `endtrigger()`, `endprogram()` primitives. |
| `analyze_traces.py` | reads 256 traces per encoder; computes per-bit signal amplitude, per-bit Pearson correlation, leaking-cycle counts; writes plots. |
| `plots/` | cached plots from the last successful run. |
| `run_table5.sh` | one-command reproduction wrapper. |
| `versions.txt` | generated by `run_table5.sh`; records every tool version used. |
| `table5.txt` | generated by `run_table5.sh`; mirrors paper Table 5 + comparison ratio. |

## Interpreting the output

The analysis script's headline block prints, e.g.:

```
  Metric                            | PermNet-RM  | BIT0MASK
  ----------------------------------+-------------+-----------
  Trace length (cycles)             |         ... |      ...
  Max signal amplitude              |         ... |      ...
  Mean signal amplitude             |         ... |      ...
  Signal ratio (BIT0MASK/PermNet)   |             |   ....x
  Max per-bit |r| (noise-free)      |      ....   |    ....
```

These are the Table 5 numbers. The "Signal ratio" is the multiplier the
paper reports. Note the known residual leakage on bit 6 — the per-bit
amplitude bar chart (`plots/per_bit_signal_amplitude.png`) shows m6's bar
for PermNet-RM close to the BIT0MASK peak. This is not a regression; it
is the 32-bit decomposition effect documented in paper §5.5 and in
[LIMITATIONS.md](../LIMITATIONS.md).

## Honest notes on the masked-d=1 harness

- Each of the 256 ELMO traces uses exactly one pseudo-random share, drawn
  from a fixed-seed linear congruential PRNG inside the trigger window.
  The seed is compile-time so the whole run is bit-deterministic. This is
  sufficient to break the statistical coupling between `m` and per-cycle
  power in ELMO's Hamming-weight model, but it is NOT a TVLA-grade
  evaluation. A proper TVLA sweep would run many trials per message with
  fresh shares and compare fixed-vs-random cohorts. Adding that is
  straightforward but is not wired up in this pack.
- The LCG is there only to break per-message correlation. It is NOT
  cryptographically secure. Production code must draw each share from a
  cryptographic RNG per call — see `source/permnet_rm17_masked_d1.c`.

## Scope: why there is no Cortex-M4 simulator here

- **ELMO** targets Cortex-M0 only. The power model was calibrated on an M0
  core and the emulator is a custom Thumbulator extension of that family.
- **GILES** (`sca-research/GILES`) integrates the ELMO power model into a
  leakage-assessment tool but still targets M0. Latest release is June
  2019; not actively maintained.
- **Rosita / Rosita++** build on top of ELMO/ELMO*. The ROSITA paper
  explicitly notes the ELMO* model "may not be suitable for more advanced
  processors like the Cortex-M4."
- There is no maintained, publicly available leakage simulator of
  comparable calibration for Cortex-M4 as of this writing.

The Jeon et al. attack targets an STM32F303 (Cortex-M4). Reproducing the
ELMO table in this pack says nothing direct about the attack on a
physical M4 device. The only credible route to an M4 claim is real
hardware measurement (ChipWhisperer + STM32F303 or STM32F415), which is
Part 4.3 of the project roadmap and is not yet done.
