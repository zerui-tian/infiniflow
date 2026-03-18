#!/usr/bin/env python3
import os
import pandas as pd
import matplotlib.pyplot as plt

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

metrics_root = os.path.join(REPO_ROOT, "ns-3/simulator/result/data/14-evaluation/threshold")
figure_root = os.path.join(REPO_ROOT, "ns-3/simulator/result/graph")
os.makedirs(figure_root, exist_ok=True)

metric = "threshold"

# =========================
# CSV → Flow 映射
# =========================
FILE_FLOW_MAP = {
    "node8_port5_q2.csv": "flow1",
    "node8_port5_q3.csv": "flow2",
    "node8_port5_q4.csv": "flow3",
    "node8_port5_q5.csv": "flow4",
}

FLOW_ORDER = ["flow1", "flow2", "flow3", "flow4"]

# =========================
# 参数
# =========================
BUFFER_SIZE = 2 * 100 * 3000 * 2 / 8   # B

# =========================
# 颜色
# =========================
FLOW_COLORS = {
    "flow1": "#2274A5",
    "flow2": "#32936F",
    "flow3": "#FFBF00",
    "flow4": "#C00000",
}

# =========================
# 虚线样式 + 图层顺序（关键）
# =========================
FLOW_LINESTYLE = {
    "flow1": (0, (2, 2)),     # 最短
    "flow2": (0, (4, 2)),
    "flow3": (0, (6, 3)),
    "flow4": (0, (10, 4)),    # 最长
}

FLOW_ZORDER = {
    "flow1": 4,
    "flow2": 3,
    "flow3": 2,
    "flow4": 1,
}

# =========================
# 绘图函数
# =========================
def plot_threshold_waveform_by_flow(metric_root, save_path):

    plt.rcParams.update({
        "font.size": 17,
        "axes.labelsize": 17,
        "xtick.labelsize": 17,
        "ytick.labelsize": 17,
    })

    plt.figure(figsize=(4.5, 3))

    # ---------- 原始数据波形 ----------
    for file_name, flow_name in FILE_FLOW_MAP.items():
        df = pd.read_csv(os.path.join(metric_root, file_name))

        time_us = df["Time"].to_numpy() / 1e3
        values = df[metric].to_numpy() / BUFFER_SIZE

        plt.plot(
            time_us,
            values,
            color=FLOW_COLORS[flow_name],
            linewidth=2,
            linestyle=FLOW_LINESTYLE[flow_name],
            zorder=FLOW_ZORDER[flow_name],
            label=flow_name
        )

    # ---------- 坐标与样式 ----------
    plt.xlabel("Time (μs)")
    plt.ylabel("Normalized Threshold", labelpad=25)
    plt.xlim(0, 500)
    plt.ylim(0, 1.05)

    # ---------- 参考线 ----------
    plt.axhline(
        y=0.05,
        color="black",
        linestyle="--",
        linewidth=1
    )
    plt.text(
        250, 0.07,
        "0.1 × Per-hop BDP",
        ha="center",
        va="bottom",
        fontsize=14
    )

    ax = plt.gca()
    ax.yaxis.set_label_coords(-0.15, 0.42)
    ax.tick_params(axis="x", pad=6)
    plt.grid(True, linestyle="--", alpha=0.5)

    # ---------- 图例 ----------
    plt.legend(
        loc="upper center",
        bbox_to_anchor=(0.5, 1),
        ncol=2,
        frameon=False,
        fontsize=17
    )

    plt.savefig(save_path, dpi=300, bbox_inches="tight")
    plt.close()

    print(f"✅ Saved to {save_path}")

# =========================
# 主入口
# =========================
if __name__ == "__main__":
    save_path = os.path.join(
        figure_root, "14-evaluation-pervc-threshold.png"
    )
    plot_threshold_waveform_by_flow(metrics_root, save_path)
