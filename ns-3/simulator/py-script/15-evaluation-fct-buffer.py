#!/usr/bin/env python3
import os
import numpy as np
import matplotlib.pyplot as plt

plt.rcParams.update({
    "font.size": 17,
    "axes.titlesize": 17,
    "axes.labelsize": 17,
    "xtick.labelsize": 17,
    "ytick.labelsize": 17,
    "legend.fontsize": 17,
})

BUFFER_LIST = [
    1600, 1920, 2240, 2560, 2880, 3200,
    3520, 3840, 4480, 4800, 5120, 5760, 6080, 6400
]
X_MARK = [25, 50, 75, 100]  # per-port buffer kB to highlight

VC_FIXED = 4096
LOAD_LIST = ["W4_load40", "W4_load80"]

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

STATIC_BASE_DIR =  os.path.join(REPO_ROOT, "ns-3/simulator/result/data/15-evaluation")
OUTPUT_DIR =  os.path.join(REPO_ROOT, "ns-3/simulator/result/graph")
os.makedirs(OUTPUT_DIR, exist_ok=True)

AVG_BASELINE = {"W4_load40": 1.46, "W4_load80": 2.35}
P99_BASELINE = {"W4_load40": 5.3, "W4_load80": 11.6}

COLORS = {"W4_load40": "#1f77b4", "W4_load80": "#C00000"}
LOAD_LABEL = {"W4_load40": "40% Intensity", "W4_load80": "80% Intensity"}
IDEAL_LABEL = {"W4_load40": "Ideal 40%", "W4_load80": "Ideal 80%"}

def read_fct_slowdowns(fct_path):
    if not os.path.exists(fct_path):
        return []
    vals = []
    with open(fct_path, "r") as f:
        for line in f:
            if not line.strip():
                continue
            cols = line.split()
            vals.append(max(1.0, float(cols[6])/float(cols[7])))
    return vals

def calc_mean_fct_slowdown(fct_path):
    vals = read_fct_slowdowns(fct_path)
    return np.mean(vals) if vals else np.nan

def calc_p99_fct_slowdown(fct_path):
    vals = read_fct_slowdowns(fct_path)
    return np.percentile(vals, 99) if vals else np.nan

def get_fct_file(load, buffer):
    return f"{STATIC_BASE_DIR}/{load}/fct/buffer/buf{buffer}_vc{VC_FIXED}.txt"

print("[INFO] Plotting Avg & P99 FCT Slowdown (Buffer Variation)")

fig, (ax_avg, ax_p99) = plt.subplots(1, 2, figsize=(6.8, 3.5))
x_all = np.array(BUFFER_LIST) / 64.0  # per-port buffer in kB

for load in LOAD_LIST:
    y_avg = np.array([calc_mean_fct_slowdown(get_fct_file(load, b)) for b in BUFFER_LIST])
    y_p99 = np.array([calc_p99_fct_slowdown(get_fct_file(load, b)) for b in BUFFER_LIST])
    mask = ~np.isnan(y_avg)

    # ---------- Avg ----------
    ax_avg.plot(
        x_all[mask],
        y_avg[mask],
        linewidth=2,
        color=COLORS[load],
        label=LOAD_LABEL[load] 
    )
    ax_avg.axhline(y=AVG_BASELINE[load], color=COLORS[load], linestyle="--", linewidth=2, alpha=0.9)
    ax_avg.text(
        0.08, 0.97,
        "Avg.",
        transform=ax_avg.transAxes,
        fontsize=17,
        ha="left",
        va="top"
    )
    ax_avg.text(
        np.mean(X_MARK),
        AVG_BASELINE[load] - 0.15,
        IDEAL_LABEL[load],
        color=COLORS[load],
        ha="center",
        va="top",
        fontsize=17
    )
    
    for x in X_MARK:
        buf_val = x * 64 
        if buf_val in BUFFER_LIST:
            idx = BUFFER_LIST.index(buf_val)
            if not np.isnan(y_avg[idx]):
                ax_avg.scatter(x, y_avg[idx], s=40, color=COLORS[load], zorder=5)

    # ---------- P99 ----------
    ax_p99.plot(
        x_all[mask],
        y_p99[mask],
        linewidth=2,
        color=COLORS[load]
    )
    ax_p99.axhline(y=P99_BASELINE[load], color=COLORS[load], linestyle="--", linewidth=2, alpha=0.9)
    ax_p99.text(
        np.mean(X_MARK),
        P99_BASELINE[load] - 1.0,
        IDEAL_LABEL[load],
        color=COLORS[load],
        ha="center",
        va="top",
        fontsize=17
    )
    ax_p99.text(
        0.08, 0.97,
        "99pct",
        transform=ax_p99.transAxes,
        fontsize=17,
        ha="left",
        va="top"
    )
    for x in X_MARK:
        buf_val = x * 64
        if buf_val in BUFFER_LIST:
            idx = BUFFER_LIST.index(buf_val)
            if not np.isnan(y_p99[idx]):
                ax_p99.scatter(x, y_p99[idx], s=40, color=COLORS[load], zorder=5)

for ax in (ax_avg, ax_p99):
    ax.set_xticks(X_MARK)
    ax.set_xticklabels([str(v) for v in X_MARK]) 
    ax.grid(True, linestyle="--", alpha=0.3)

ax_avg.set_ylabel("Avg. FCT Slowdown")
ax_avg.set_ylim(1, 4.5)

ax_p99.set_ylabel("P99 FCT Slowdown")
ax_p99.set_ylim(1, 25)
ax_p99.yaxis.set_label_position("right")
ax_p99.yaxis.tick_right()

fig.supxlabel("Per-Port Buffer Size (KB)", fontsize=17)

plt.subplots_adjust(
    left=0.13,
    right=0.87, 
    bottom=0.3,
    top=0.90,  
    wspace=0.1  
)

out_fig = os.path.join(OUTPUT_DIR, "15-evaluation-fct-buffer.png")
plt.savefig(out_fig, dpi=300)
plt.show()
print(f"[OK] Saved: {out_fig}")
