#!/usr/bin/env python3
"""
analyze_traces.py -- Compare ELMO power traces between PermNet-RM and BIT0MASK.

Reads 256 power traces from each encoder (one per 8-bit message input) and
analyses the side-channel leakage using the metric that matters for the
Jeon et al. (ePrint 2026/071) attack: per-cycle correlation with INDIVIDUAL
message bits.

Jeon's attack recovers individual bits of m from the Hamming weight of the
mask variable (0x00000000 vs 0xFFFFFFFF).  The correct leakage metric is
therefore max |corr(bit_i, power_cycle_j)| across all bits and cycles.

Usage:
    python3 analyze_traces.py <permnet_trace_dir> <bitmask_trace_dir> [output_dir]
"""

import os
import sys
import numpy as np

# ---------------------------------------------------------------------------
#  Load traces
# ---------------------------------------------------------------------------

def load_traces(trace_dir, n=256):
    """Load n ELMO trace files from a directory."""
    traces = []
    for i in range(1, n + 1):
        path = os.path.join(trace_dir, f"trace{i:05d}.trc")
        with open(path) as f:
            samples = [float(line.strip()) for line in f if line.strip()]
        traces.append(np.array(samples))
    return traces


def hamming_weight(x):
    return bin(x).count('1')


# ---------------------------------------------------------------------------
#  Per-bit correlation analysis (the Jeon-relevant metric)
# ---------------------------------------------------------------------------

def per_bit_analysis(traces, n=256):
    """
    For each message bit (0..7) and each power trace cycle, compute:
    1. Pearson correlation between bit value and power at that cycle
    2. Signal amplitude: |E[power|bit=1] - E[power|bit=0]|

    In ELMO's noise-free model, correlation is always high for any
    non-zero signal.  The amplitude captures the actual exploitability:
    BIT0MASK produces 32-bit HW swings (0 vs 0xFFFFFFFF) while
    PermNet-RM produces 1-bit HW changes (0 vs 1 at a single position).

    Returns dict with correlation and amplitude arrays.
    """
    trace_matrix = np.array(traces)  # (256, trace_len)
    trace_len = trace_matrix.shape[1]

    # Extract individual bit values for each input 0..255
    bits = np.zeros((n, 8), dtype=np.float64)
    for i in range(n):
        for b in range(8):
            bits[i, b] = (i >> b) & 1

    bit_corr = np.zeros((8, trace_len))
    bit_signal = np.zeros((8, trace_len))  # signal amplitude

    for b in range(8):
        mask0 = bits[:, b] == 0
        mask1 = bits[:, b] == 1
        for c in range(trace_len):
            col = trace_matrix[:, c]
            if np.std(col) < 1e-15:
                bit_corr[b, c] = 0.0
                bit_signal[b, c] = 0.0
            else:
                bit_corr[b, c] = np.corrcoef(bits[:, b], col)[0, 1]
                bit_signal[b, c] = abs(np.mean(col[mask1]) - np.mean(col[mask0]))

    return {
        'corr': bit_corr,
        'signal': bit_signal,
        'max_corr_per_bit': np.nanmax(np.abs(bit_corr), axis=1),
        'max_corr_per_cycle': np.nanmax(np.abs(bit_corr), axis=0),
        'max_signal_per_bit': np.nanmax(bit_signal, axis=1),
        'max_signal_per_cycle': np.nanmax(bit_signal, axis=0),
        'mean_signal_per_bit': np.nanmean(bit_signal, axis=1),
    }


# ---------------------------------------------------------------------------
#  Analysis
# ---------------------------------------------------------------------------

def analyze(traces, label):
    """Compute per-trace metrics, per-bit correlation, and signal amplitude."""
    n = len(traces)
    hw = np.array([hamming_weight(i) for i in range(n)])
    total_power = np.array([np.sum(t) for t in traces])
    mean_power = np.array([np.mean(t) for t in traces])
    trace_len = len(traces[0])

    # HW correlation (context only)
    corr_hw = np.corrcoef(hw, total_power)[0, 1]

    # Per-bit analysis
    pba = per_bit_analysis(traces)

    print(f"\n{'='*70}")
    print(f"  {label}")
    print(f"{'='*70}")
    print(f"  Trace length:  {trace_len} cycles")
    print(f"  Total power:   [{total_power.min():.2f}, {total_power.max():.2f}]")
    print()
    print(f"  --- SIGNAL AMPLITUDE (noise-free, key metric for ELMO) ---")
    print(f"  Max |E[power|bit=1] - E[power|bit=0]| per bit:")
    for b in range(8):
        sig = pba['max_signal_per_bit'][b]
        bar = '#' * min(int(sig / 10), 50)
        print(f"    bit {b}: {sig:12.4f}  {bar}")
    print(f"  Overall max signal:  {np.nanmax(pba['max_signal_per_bit']):.4f}")
    print(f"  Mean max signal:     {np.nanmean(pba['max_signal_per_bit']):.4f}")
    print()
    print(f"  --- CORRELATION (high in noise-free sim for any nonzero signal) ---")
    print(f"  Per-bit max |r|:")
    for b in range(8):
        print(f"    bit {b}: {pba['max_corr_per_bit'][b]:.6f}")
    print(f"  Leaking cycles (|r| > 0.3): {int(np.sum(pba['max_corr_per_cycle'] > 0.3))}/{trace_len}")

    return {
        'total_power': total_power,
        'mean_power': mean_power,
        'hw': hw,
        'corr_hw': corr_hw,
        'pba': pba,
        'trace_len': trace_len,
    }


# ---------------------------------------------------------------------------
#  Plotting
# ---------------------------------------------------------------------------

def try_plot(permnet_stats, bitmask_stats, permnet_traces, bitmask_traces, output_dir):
    """Generate comparison plots."""
    try:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
    except ImportError:
        print("\n[matplotlib not available — skipping plots]")
        return

    os.makedirs(output_dir, exist_ok=True)
    hw = permnet_stats['hw']
    pn = permnet_stats['pba']
    bm = bitmask_stats['pba']

    # ===== Figure 1: SIGNAL AMPLITUDE HEATMAPS (THE KEY FIGURE) =====
    # Use common color scale so the magnitude difference is visible
    vmax = max(np.nanmax(pn['signal']), np.nanmax(bm['signal']))

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 6))

    im1 = ax1.imshow(pn['signal'], aspect='auto', cmap='hot_r',
                      vmin=0, vmax=vmax, interpolation='nearest')
    ax1.set_ylabel('Message bit', fontsize=11)
    ax1.set_yticks(range(8))
    ax1.set_title(f"PermNet-RM: Signal Amplitude  "
                  f"(max = {np.nanmax(pn['max_signal_per_bit']):.1f})",
                  fontsize=12)
    plt.colorbar(im1, ax=ax1, label='|E[P|b=1] - E[P|b=0]|')

    im2 = ax2.imshow(bm['signal'], aspect='auto', cmap='hot_r',
                      vmin=0, vmax=vmax, interpolation='nearest')
    ax2.set_xlabel('Cycle index', fontsize=11)
    ax2.set_ylabel('Message bit', fontsize=11)
    ax2.set_yticks(range(8))
    ax2.set_title(f"BIT0MASK: Signal Amplitude  "
                  f"(max = {np.nanmax(bm['max_signal_per_bit']):.1f})",
                  fontsize=12)
    plt.colorbar(im2, ax=ax2, label='|E[P|b=1] - E[P|b=0]|')

    fig.tight_layout()
    path = os.path.join(output_dir, 'signal_amplitude_heatmap.png')
    fig.savefig(path, dpi=200, bbox_inches='tight')
    print(f"  Saved: {path}")
    plt.close(fig)

    # ===== Figure 2: Max signal per bit (bar chart) =====
    fig, ax = plt.subplots(figsize=(10, 5))
    x = np.arange(8)
    w = 0.35
    ax.bar(x - w/2, pn['max_signal_per_bit'], w, color='steelblue',
           alpha=0.8, label='PermNet-RM')
    ax.bar(x + w/2, bm['max_signal_per_bit'], w, color='firebrick',
           alpha=0.8, label='BIT0MASK')
    ax.set_xlabel('Message Bit Index', fontsize=12)
    ax.set_ylabel('Max Signal Amplitude', fontsize=12)
    ax.set_title('Per-Bit Signal Amplitude: |E[power|bit=1] - E[power|bit=0]|', fontsize=13)
    ax.set_xticks(x)
    ax.set_xticklabels([f'$m_{i}$' for i in range(8)])
    ax.legend(fontsize=10)

    # Add ratio annotation
    ratio = np.nanmax(bm['max_signal_per_bit']) / max(np.nanmax(pn['max_signal_per_bit']), 1e-10)
    ax.annotate(f'BIT0MASK/PermNet signal ratio: {ratio:.1f}x',
                xy=(0.5, 0.95), xycoords='axes fraction',
                ha='center', fontsize=11, fontweight='bold',
                bbox=dict(boxstyle='round,pad=0.3', facecolor='lightyellow'))

    fig.tight_layout()
    path = os.path.join(output_dir, 'per_bit_signal_amplitude.png')
    fig.savefig(path, dpi=200, bbox_inches='tight')
    print(f"  Saved: {path}")
    plt.close(fig)

    # ===== Figure 3: Per-cycle max signal envelope =====
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 6), sharey=True)

    ax1.fill_between(range(len(pn['max_signal_per_cycle'])),
                     pn['max_signal_per_cycle'],
                     color='steelblue', alpha=0.4)
    ax1.plot(pn['max_signal_per_cycle'], color='steelblue', linewidth=0.8)
    ax1.set_ylabel('Max signal across bits', fontsize=11)
    ax1.set_title('PermNet-RM: Per-Cycle Signal Envelope', fontsize=12)

    ax2.fill_between(range(len(bm['max_signal_per_cycle'])),
                     bm['max_signal_per_cycle'],
                     color='firebrick', alpha=0.4)
    ax2.plot(bm['max_signal_per_cycle'], color='firebrick', linewidth=0.8)
    ax2.set_xlabel('Cycle Index', fontsize=11)
    ax2.set_ylabel('Max signal across bits', fontsize=11)
    ax2.set_title('BIT0MASK: Per-Cycle Signal Envelope', fontsize=12)

    fig.tight_layout()
    path = os.path.join(output_dir, 'per_cycle_signal_envelope.png')
    fig.savefig(path, dpi=200, bbox_inches='tight')
    print(f"  Saved: {path}")
    plt.close(fig)

    # ===== Figure 4: Correlation heatmaps (secondary) =====
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 6))

    im1 = ax1.imshow(pn['corr'], aspect='auto', cmap='RdBu_r',
                      vmin=-1, vmax=1, interpolation='nearest')
    ax1.set_ylabel('Message bit', fontsize=11)
    ax1.set_yticks(range(8))
    ax1.set_title('PermNet-RM: Per-Bit Correlation (note: noise-free)', fontsize=12)
    plt.colorbar(im1, ax=ax1, label='Pearson r')

    im2 = ax2.imshow(bm['corr'], aspect='auto', cmap='RdBu_r',
                      vmin=-1, vmax=1, interpolation='nearest')
    ax2.set_xlabel('Cycle index', fontsize=11)
    ax2.set_ylabel('Message bit', fontsize=11)
    ax2.set_yticks(range(8))
    ax2.set_title('BIT0MASK: Per-Bit Correlation (note: noise-free)', fontsize=12)
    plt.colorbar(im2, ax=ax2, label='Pearson r')

    fig.tight_layout()
    path = os.path.join(output_dir, 'per_bit_correlation_heatmap.png')
    fig.savefig(path, dpi=200, bbox_inches='tight')
    print(f"  Saved: {path}")
    plt.close(fig)

    # ===== Figure 5: Mean trace by HW bucket =====
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 7))
    cmap = plt.cm.viridis

    for h in range(9):
        mask = hw == h
        if np.any(mask):
            traces_h = np.array([permnet_traces[i] for i in range(256) if mask[i]])
            ax1.plot(traces_h.mean(axis=0), color=cmap(h/8), alpha=0.8,
                     label=f'HW={h}', linewidth=1.2)
    ax1.set_ylabel('Power', fontsize=11)
    ax1.set_title('PermNet-RM: Mean Trace by HW Bucket', fontsize=12)
    ax1.legend(fontsize=8, ncol=3, loc='upper right')

    for h in range(9):
        mask = hw == h
        if np.any(mask):
            traces_h = np.array([bitmask_traces[i] for i in range(256) if mask[i]])
            ax2.plot(traces_h.mean(axis=0), color=cmap(h/8), alpha=0.8,
                     label=f'HW={h}', linewidth=1.2)
    ax2.set_xlabel('Cycle', fontsize=11)
    ax2.set_ylabel('Power', fontsize=11)
    ax2.set_title('BIT0MASK: Mean Trace by HW Bucket', fontsize=12)
    ax2.legend(fontsize=8, ncol=3, loc='upper right')

    fig.tight_layout()
    path = os.path.join(output_dir, 'mean_traces_by_hw.png')
    fig.savefig(path, dpi=200, bbox_inches='tight')
    print(f"  Saved: {path}")
    plt.close(fig)


# ---------------------------------------------------------------------------
#  Main
# ---------------------------------------------------------------------------

def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <permnet_traces> <bitmask_traces> [output_dir]")
        sys.exit(1)

    permnet_dir = sys.argv[1]
    bitmask_dir = sys.argv[2]
    output_dir = sys.argv[3] if len(sys.argv) > 3 else 'plots'

    print("Loading PermNet-RM traces...")
    permnet_traces = load_traces(permnet_dir)
    print("Loading BIT0MASK traces...")
    bitmask_traces = load_traces(bitmask_dir)

    permnet_stats = analyze(permnet_traces, "PermNet-RM (butterfly encoder)")
    bitmask_stats = analyze(bitmask_traces, "BIT0MASK (vulnerable encoder)")

    # Summary
    pn = permnet_stats['pba']
    bm = bitmask_stats['pba']

    pn_sig = np.nanmax(pn['max_signal_per_bit'])
    bm_sig = np.nanmax(bm['max_signal_per_bit'])
    ratio = bm_sig / max(pn_sig, 1e-10)

    print(f"\n{'='*70}")
    print(f"  JEON ATTACK-RELEVANT COMPARISON")
    print(f"{'='*70}")
    print(f"  ELMO produces noise-free traces.  In noise-free data, Pearson |r|")
    print(f"  is ~1 for ANY nonzero signal.  The attack-relevant metric is")
    print(f"  signal AMPLITUDE: how much does power change between bit=0 and")
    print(f"  bit=1?  Larger amplitude = easier to extract from noisy real")
    print(f"  measurements.  BIT0MASK's mask flips 32 register bits (HW 0 vs 32);")
    print(f"  PermNet-RM's bit placement flips 1 register bit (HW 0 vs 1).")
    print()
    print(f"  Metric                            | PermNet-RM  | BIT0MASK")
    print(f"  ----------------------------------+-------------+-----------")
    print(f"  Trace length (cycles)             | {permnet_stats['trace_len']:>11} | {bitmask_stats['trace_len']:>9}")
    print(f"  Max signal amplitude              | {pn_sig:>11.2f} | {bm_sig:>9.2f}")
    print(f"  Mean signal amplitude             | {np.nanmean(pn['max_signal_per_bit']):>11.2f} | {np.nanmean(bm['max_signal_per_bit']):>9.2f}")
    print(f"  Signal ratio (BIT0MASK/PermNet)   | {'':>11} | {ratio:>8.1f}x")
    print(f"  Max per-bit |r| (noise-free)      | {np.nanmax(pn['max_corr_per_bit']):>11.4f} | {np.nanmax(bm['max_corr_per_bit']):>9.4f}")
    print()
    print(f"  BIT0MASK signal is {ratio:.1f}x larger than PermNet-RM.")
    print(f"  Under realistic noise (sigma), the number of traces needed to")
    print(f"  recover a bit scales as (sigma/signal)^2.  This means BIT0MASK")
    print(f"  requires ~{ratio**2:.0f}x fewer traces than PermNet-RM for the same")
    print(f"  recovery confidence.  Jeon achieves single-trace recovery of")
    print(f"  BIT0MASK; PermNet-RM would require ~{ratio**2:.0f} traces for equivalent SNR.")

    print(f"\nGenerating plots...")
    try_plot(permnet_stats, bitmask_stats, permnet_traces, bitmask_traces, output_dir)
    print("\nDone.")


if __name__ == '__main__':
    main()
