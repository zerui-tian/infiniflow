#!/usr/bin/env python3
import os
import numpy as np
import matplotlib.pyplot as plt

# ================== 全局字体配置（统一 17） ==================
plt.rcParams.update({
    "font.size": 17,
    "axes.titlesize": 17,
    "axes.labelsize": 17,
    "xtick.labelsize": 17,
    "ytick.labelsize": 17,
    "legend.fontsize": 17
})

BUFFER_LIST = [20, 50, 100, 200, 500, 1000]
VC_LIST = [8, 16, 32, 64, 128, 256, 512, 1024]
# ALGO_LIST = ["PFC"]
ALGO_LIST = ["CBFC", "PFC", "DCQCN", "HPCC"]

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

DATA_DIR = os.path.join(REPO_ROOT,"ns-3/simulator/result/data/3-motivation")
OUTPUT_DIR = os.path.join(REPO_ROOT, "ns-3/simulator/result/graph")

os.makedirs(OUTPUT_DIR, exist_ok=True)

# ================== 工具函数 ==================
def calc_mean_slowdown(fct_path):
    if not os.path.exists(fct_path):
        print(f"[WARN] File not found, skip: {fct_path}")
        return np.nan

    slowdowns = []
    try:
        with open(fct_path, "r") as f:
            for line in f:
                if not line.strip():
                    continue
                cols = line.split()
                raw_slowdown = float(cols[6]) / float(cols[7])
                slowdown = max(1.0, raw_slowdown)
                slowdowns.append(slowdown)
    except Exception as e:
        print(f"[WARN] Parse error, skip: {fct_path}, err={e}")
        return np.nan

    if not slowdowns:
        return np.nan

    return np.mean(slowdowns)


def get_fct_file(algo, buffer, vc):
    base_name = f"buf{buffer}_vc{vc}"
    if algo == "CBFC":
        return f"{DATA_DIR}/{base_name}_cbfc.txt"
    elif algo == "PFC":
        return f"{DATA_DIR}/{base_name}_pfc.txt"
    else:  # DCQCN, HPCC
        return f"{DATA_DIR}/{base_name}_{algo.lower()}.txt"

# ================== 颜色配置 ==================
cmap = plt.cm.viridis
num_buf = len(BUFFER_LIST)
colors = [cmap(1.0 - i / (num_buf - 1)) for i in range(num_buf)]

# ================== 主循环：逐算法画图 ==================
for ALGO in ALGO_LIST:
    print(f"[INFO] Processing {ALGO}")

    # ---------- 数据收集 ----------
    data = {}
    for buffer in BUFFER_LIST:
        vals = []
        for vc in VC_LIST:
            fct_file = get_fct_file(ALGO, buffer, vc)
            vals.append(calc_mean_slowdown(fct_file))
        data[buffer] = np.array(vals)

    # ---------- 绘图 ----------
    plt.figure(figsize=(4.5, 3))

    for idx, buffer in enumerate(BUFFER_LIST):
        x = np.array(VC_LIST)
        y = data[buffer].copy()

        # ===== 核心过滤逻辑 =====
        y[(y <= 1.5) | (y > 7)] = np.nan

        plt.plot(
            x,
            y,
            marker="o",
            linewidth=1.8,
            color=colors[idx],
            label=f"{buffer} MB"
        )

    # ---- 添加红色虚线和标注 ----
    y_ideal = 1.679
    plt.axhline(y=y_ideal, color='red', linestyle='--', linewidth=2)

    # 获取 X 轴的中心点
    x_mid = np.sqrt(VC_LIST[0] * VC_LIST[-1])

    plt.text(
        x=x_mid,
        y=y_ideal - 0.1,
        s="ideal transmission",
        color='red',
        fontsize=17,
        ha='center',
        va='top'
    )
    
    plt.xlabel("The Number of VCs")
    plt.ylabel("Avg. FCT Slowdown")
    plt.yticks([2, 4, 6])
    plt.xscale("log", base=2)
    plt.xticks(VC_LIST, [str(v) for v in VC_LIST], rotation=45, ha="center")

    ax = plt.gca()
    ax.tick_params(axis="x", pad=0.5)

    # ===== 固定 y 轴范围 =====
    ax.set_ylim(1, 7)

    plt.grid(True, linestyle="--", alpha=0.4)

    OUTPUT_FIG = os.path.join(
        OUTPUT_DIR,
        f"3-motivation-{ALGO}_fct_slowdown.png"
    )
    plt.savefig(OUTPUT_FIG, bbox_inches="tight")
    plt.close()

    print(f"[OK] Figure saved: {OUTPUT_FIG}")

# ================== 单独生成 Legend ==================
legend_fig = plt.figure(figsize=(4.5 * 4, 1.2))
legend_ax = legend_fig.add_subplot(111)

handles = []
labels = []

for idx, buffer in enumerate(BUFFER_LIST):
    h, = legend_ax.plot(
        [],
        [],
        marker="o",
        linewidth=1.8,
        color=colors[idx],
        label=f"{buffer} MB"
    )
    handles.append(h)
    labels.append(f"{buffer} MB")

legend = legend_ax.legend(
    handles,
    labels,
    ncol=6,
    frameon=False,
    loc="center"
)

legend_ax.axis("off")

legend_fig.canvas.draw()
renderer = legend_fig.canvas.get_renderer()
bbox = legend.get_window_extent(renderer=renderer)
bbox_fig = bbox.transformed(legend_fig.transFigure.inverted())

legend_fig.text(
    bbox_fig.x0 - 0.01,
    (bbox_fig.y0 + bbox_fig.y1) / 2,
    "Total Buffer Size:",
    ha="right",
    va="center",
    fontsize=17
)

LEGEND_FIG = os.path.join(
    OUTPUT_DIR,
    "3-motivation-legend.png"
)
legend_fig.savefig(LEGEND_FIG, bbox_inches="tight")
plt.close(legend_fig)

print(f"[OK] Legend saved: {LEGEND_FIG}")