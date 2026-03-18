#!/usr/bin/env python3
import os
import numpy as np
import matplotlib.pyplot as plt

# ================== 全局字体配置 ==================
plt.rcParams.update({
    "font.size": 17,
    "axes.titlesize": 17,
    "axes.labelsize": 17,
    "xtick.labelsize": 17,
    "ytick.labelsize": 17,
    "legend.fontsize": 17,
})

# ================== 基本配置 ==================
VC_LIST = [8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192]
LOAD_LIST = ["W4_load40", "W4_load80"]
BUFFER_SIZE = 6400

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

STATIC_BASE_DIR =  os.path.join(REPO_ROOT, "ns-3/simulator/result/data/15-evaluation")
OUTPUT_DIR =  os.path.join(REPO_ROOT, "ns-3/simulator/result/graph")
os.makedirs(OUTPUT_DIR, exist_ok=True)

# ================== Baseline ==================
AVG_BASELINE = {"W4_load40": 1.46, "W4_load80": 2.35}
P99_BASELINE = {"W4_load40": 5.3, "W4_load80": 11.6}

# ================== 样式 ==================
COLORS = {"W4_load40": "#1f77b4", "W4_load80": "#C00000"}
LOAD_LABEL = {"W4_load40": "40% Intensity", "W4_load80": "80% Intensity"}
IDEAL_LABEL = {"W4_load40": "Ideal 40%", "W4_load80": "Ideal 80%"}

# ================== 工具函数 ==================
def read_fct_slowdowns(fct_path):
    if not os.path.exists(fct_path):
        return []
    vals = []
    with open(fct_path, "r") as f:
        for line in f:
            if not line.strip(): continue
            cols = line.split()
            vals.append(max(1.0, float(cols[6])/float(cols[7])))
    return vals

def calc_mean_fct_slowdown(fct_path):
    vals = read_fct_slowdowns(fct_path)
    return np.mean(vals) if vals else np.nan

def calc_p99_fct_slowdown(fct_path):
    vals = read_fct_slowdowns(fct_path)
    return np.percentile(vals, 99) if vals else np.nan

def get_fct_file(load, vc):
    return f"{STATIC_BASE_DIR}/{load}/fct/vcs/buf{BUFFER_SIZE}_vc{vc}.txt"

# ================== 绘图 ==================
print("[INFO] Plotting Avg & P99 FCT Slowdown (VC Variation)")

fig, (ax_avg, ax_p99) = plt.subplots(1, 2, figsize=(6.8, 3.5))

# 横坐标只标记这几个 VC
XTICKS = [8, 32, 128, 512, 2048, 8192]
x_center = np.sqrt(XTICKS[0] * XTICKS[-1])
for load in LOAD_LIST:
    y_avg = [calc_mean_fct_slowdown(get_fct_file(load, vc)) for vc in VC_LIST]
    y_p99 = [calc_p99_fct_slowdown(get_fct_file(load, vc)) for vc in VC_LIST]
    
    vcs = np.array(VC_LIST)
    y_avg = np.array(y_avg)
    y_p99 = np.array(y_p99)
    mask = ~np.isnan(y_avg)

    # ---------- Avg ----------
    ax_avg.plot(vcs[mask], y_avg[mask], linewidth=2, color=COLORS[load], label=LOAD_LABEL[load])
    ax_avg.axhline(y=AVG_BASELINE[load], color=COLORS[load], linestyle="--", linewidth=2, alpha=0.9)
    ax_avg.text(x_center, AVG_BASELINE[load] - 0.15, IDEAL_LABEL[load],
                color=COLORS[load], ha="center", va="top", fontsize=17)
    ax_avg.text(
        0.08, 0.97,
        "Avg.",
        transform=ax_avg.transAxes,
        fontsize=17,
        ha="left",
        va="top"
    )
    # Avg markers：只在横坐标刻度位置
    for x in XTICKS:
        if x in vcs:
            idx = list(vcs).index(x)
            if not np.isnan(y_avg[idx]):
                ax_avg.scatter(x, y_avg[idx], s=50, color=COLORS[load], zorder=5)
    

    # ---------- P99 ----------
    ax_p99.plot(vcs[mask], y_p99[mask], linewidth=2, color=COLORS[load])
    ax_p99.axhline(y=P99_BASELINE[load], color=COLORS[load], linestyle="--", linewidth=2, alpha=0.9)
    ax_p99.text(x_center, P99_BASELINE[load] - 1.0, IDEAL_LABEL[load],
                color=COLORS[load], ha="center", va="top", fontsize=17)
    ax_p99.text(
        0.08, 0.97,
        "99pct",
        transform=ax_p99.transAxes,
        fontsize=17,
        ha="left",
        va="top"
    )
    # P99 markers：只在横坐标刻度位置
    for x in XTICKS:
        if x in vcs:
            idx = list(vcs).index(x)
            if not np.isnan(y_p99[idx]):
                ax_p99.scatter(x, y_p99[idx], s=50, color=COLORS[load], zorder=5)

# ================== 坐标轴 & 布局 ==================
for ax in (ax_avg, ax_p99):
    ax.set_xscale("log", base=2)
    ax.set_xticks(XTICKS)
    ax.set_xticklabels([str(v) for v in XTICKS], rotation=45)
    ax.grid(True, which="both", linestyle="--", alpha=0.3)

ax_avg.set_ylabel("Avg. FCT Slowdown")
ax_avg.set_ylim(1, 4.5)
# ax_avg.legend(frameon=False, loc="upper center", fontsize=17)

ax_p99.set_ylabel("P99 FCT Slowdown")
ax_p99.set_ylim(1, 25)
ax_p99.yaxis.set_label_position("right")
ax_p99.yaxis.tick_right()

fig.supxlabel("The Number of VCs", fontsize=17)
plt.subplots_adjust(
    left=0.13,   # 统一左边距
    right=0.87,  # 统一右边距
    bottom=0.3, # 统一底边距（留给 X 轴标题）
    top=0.90,    # 统一顶边距
    wspace=0.1  # 关键：增大子图间距，防止左图图例和右图坐标轴“打架”
)
out_fig = os.path.join(OUTPUT_DIR, "15-evaluation-fct-vcs.png")
plt.savefig(out_fig, dpi=300)
plt.show()
print(f"[OK] Saved: {out_fig}")
