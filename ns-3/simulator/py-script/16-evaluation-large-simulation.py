#!/usr/bin/env python3
import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker

plt.rcParams.update({
    'font.size': 17,
    'axes.titlesize': 17,
    'axes.labelsize': 17,
    'xtick.labelsize': 17,
    'ytick.labelsize': 17,
    'legend.fontsize': 17,
    'figure.titlesize': 17
})

WORKLOAD_FILES = ["W3_load40.txt", "W3_load80.txt", "W4_load40.txt", "W4_load80.txt"]

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

BASE_DIR = os.path.join(REPO_ROOT, "ns-3/simulator/result/data/16-large-scale")

ALGO_NAMES = ["InfiniFlow", "CBFC alone", "PFC alone", "DCQCN + PFC", "HPCC + PFC", "ExpressPass", "BFC"]
STYLES = {
    "InfiniFlow": {'color': '#C00000', 'ls': '-',  'lw': 2, 'zorder': 10},
    "PFC alone":   {'color': '#1f77b4', 'ls': '-.', 'lw': 2},
    "DCQCN + PFC": {'color': '#9467bd', 'ls': '--', 'lw': 2},
    "CBFC alone":  {'color': '#2ca02c', 'ls': '-.', 'lw': 2},
    "HPCC + PFC":  {'color': '#E83F6F', 'ls': (0, (6, 3)), 'lw': 2},
    "ExpressPass": {'color': '#8c564b', 'ls': (0, (1, 1)), 'lw': 2},
    "BFC":         {'color': '#f47F38', 'ls': (0, (3, 1, 1, 1)), 'lw': 2},
}

OUT_DIR = os.path.join(REPO_ROOT, "ns-3/simulator/result/graph")

MAX_SIZE_BYTE = 16000000
BINS = [0, 1024]; curr = 1024
while curr < MAX_SIZE_BYTE:
    curr *= 2; BINS.append(curr)
BIN_X = [b / 1024.0 for b in BINS[1:]]

def export_standalone_legend(out_dir):
    """Export a standalone horizontal legend"""
    fig_leg = plt.figure(figsize=(18, 0.4))
    handles = []
    for name in ALGO_NAMES:
        style = STYLES[name]
        line = plt.Line2D([0], [0], label=name, **style)
        handles.append(line)

    fig_leg.legend(handles=handles, labels=ALGO_NAMES, loc='center',
                   ncol=len(ALGO_NAMES), frameon=False,
                   columnspacing=1.2, handletextpad=0.5)

    plt.axis('off')
    path = os.path.join(out_dir, "legend_standalone_7_no_ideal_v2.pdf")
    plt.savefig(path, pad_inches=0.1)
    plt.close()
    print(f"[SUCCESS] Standalone legend exported: {path}")

def load_fct_data(file_path):
    sizes, slowdowns = [], []
    if not os.path.isfile(file_path):
        return np.array([]), np.array([])
    with open(file_path, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) < 8: continue
            try:
                size = int(parts[4])
                actual_fct = float(parts[6])
                ideal_fct = float(parts[7])
                if ideal_fct <= 0: continue
                slowdowns.append(max(actual_fct / ideal_fct, 1.0))
                sizes.append(size)
            except:
                continue
    return np.array(sizes), np.array(slowdowns)

def get_stats_by_bins(sizes, slowdowns):
    res_avg, res_p95, res_p99 = [], [], []
    for i in range(len(BINS)-1):
        mask = (sizes >= BINS[i]) & (sizes < BINS[i+1])
        data = slowdowns[mask]
        if len(data) > 0:
            res_avg.append(np.mean(data))
            res_p95.append(np.percentile(data, 95))
            res_p99.append(np.percentile(data, 99))
        else:
            res_avg.append(np.nan)
            res_p95.append(np.nan)
            res_p99.append(np.nan)
    return res_avg, res_p95, res_p99

def plot_final_figure(all_stats, out_path, workload_name):
    fig, axes = plt.subplots(1, 3, figsize=(9, 2.8), sharex=True, sharey=False)
    metrics = ["Avg.", "95pct", "99pct"]
    x_ticks = [1, 10, 100, 1000, 10000]
    y_ticks = [1, 2, 4, 8, 16, 32, 64]

    for i, metric in enumerate(metrics):
        ax = axes[i]
        for algo in ALGO_NAMES:
            if algo not in all_stats: continue
            y_data = all_stats[algo][i]
            style = STYLES.get(algo, {'color': 'black'})
            valid_mask = ~np.isnan(y_data)
            ax.plot(np.array(BIN_X)[valid_mask], np.array(y_data)[valid_mask], label=algo, **style)

        ax.set_xscale('log')
        ax.set_xticks(x_ticks)
        ax.xaxis.set_major_formatter(mticker.LogFormatterMathtext(base=10))
        ax.set_yscale('log', base=2)
        ax.set_ylim(0.9, 70)
        ax.set_xlim(BIN_X[0], BIN_X[-1])
        ax.set_yticks(y_ticks)
        ax.yaxis.set_major_formatter(mticker.ScalarFormatter())

        if i != 0:
            ax.set_yticklabels([])
            ax.set_ylabel("")

        ax.text(0.27, 0.96, metric, transform=ax.transAxes,
                verticalalignment='top', horizontalalignment='right',
                fontsize=17)

        ax.grid(False)
        if i == 0:
            ax.set_ylabel('FCT SlowDown')

    fig.text(0.5, 0.01, 'Flow Size (KB)', ha='center', fontsize=18)

    plt.subplots_adjust(left=0.08, right=0.99, bottom=0.19, top=0.95, wspace=0.1)
    plt.savefig(out_path, dpi=300)
    plt.close()

def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    algo_dirs = {
        "InfiniFlow": "infiniflow",
        "CBFC alone": "infiniband",
        "PFC alone": "pfc",
        "DCQCN + PFC": "dcqcn",
        "HPCC + PFC": "hpcc",
        "ExpressPass": "xpass",
        "BFC": "bfc"
    }

    for workload in WORKLOAD_FILES:
        print(f"[PROCESSING] {workload}")
        workload_stats = {}

        for algo_name in ALGO_NAMES:
            if algo_name in algo_dirs:
                algo_dir = algo_dirs[algo_name]
                fct_file = os.path.join(BASE_DIR, algo_dir, "fct", workload)

                sizes, slowdowns = load_fct_data(fct_file)
                if len(sizes) > 0:
                    workload_stats[algo_name] = get_stats_by_bins(sizes, slowdowns)
                else:
                    print(f"[WARNING] File missing or empty: {fct_file}")

        if workload_stats:
            save_name = "16-evaluation-" + workload.replace(".txt", "-fct-slowdown.png")
            save_path = os.path.join(OUT_DIR, save_name)
            plot_final_figure(workload_stats, save_path, workload)
            print(f"[DONE] {save_name}")
        else:
            print(f"[ERROR] No valid data loaded for {workload}")

if __name__ == "__main__":
    export_standalone_legend(OUT_DIR)
    main()