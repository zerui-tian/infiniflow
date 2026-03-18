#!/usr/bin/env python3
import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

metrics_root = os.path.join(REPO_ROOT, "ns-3/simulator/result/data/18-dt-limitation/inflight")
figure_root = os.path.join(REPO_ROOT, "ns-3/simulator/result/graph")

os.makedirs(figure_root, exist_ok=True)

metric = "inflight"

target_files = [
    "node8_port5_q2.csv",
    "node8_port5_q3.csv",
    "node8_port5_q4.csv",
    "node8_port5_q5.csv",
]

# =========================
# 参数
# =========================
BIN_SIZE_NS = 1000          # 1000 ns = 1 us
BUFFER_SIZE = 2 * 100 * 3000 * 2 / 8   # kB

# =========================
# 颜色（按 q2 → q5 顺序）
# =========================
COLORS = [
    '#2274A5',  # q2
    '#32936F',  # q3
    '#FFBF00',  # q4
    '#C00000',  # q5
]

def resample_by_time_bin(df, value_col, bin_size_ns):
    df = df[(df["Time"] >= 0) & (df["Time"] <= 1_000_000)].copy()
    df.loc[:, "bin"] = (df["Time"] // bin_size_ns).astype(int)

    grouped = df.groupby("bin")[value_col].mean()

    time_ns = (grouped.index + 0.5) * bin_size_ns
    time_ms = time_ns / 1e6

    return time_ms.to_numpy(), grouped.to_numpy()

def plot_stacked_inflight(metric_root, metric, file_names, save_path):

    plt.rcParams.update({
        "font.size": 17,
        "axes.labelsize": 17,
        "xtick.labelsize": 17,
        "ytick.labelsize": 17,
    })

    all_values = []
    common_time = None

    for file_name in file_names:
        df = pd.read_csv(os.path.join(metric_root, file_name))

        time_ms, values = resample_by_time_bin(
            df, value_col=metric, bin_size_ns=BIN_SIZE_NS
        )

        # inflight → buffer usage
        values = values / BUFFER_SIZE

        if common_time is None:
            common_time = time_ms
        else:
            min_len = min(len(common_time), len(values))
            common_time = common_time[:min_len]
            all_values = [v[:min_len] for v in all_values]
            values = values[:min_len]

        all_values.append(values)

    values = np.array(all_values)
    cumulative = np.cumsum(values, axis=0)

    plt.figure(figsize=(4.5, 3))

    labels = ["flow1", "flow2", "flow3", "flow4"]
    bottom = np.zeros_like(common_time)

    for i in range(len(file_names)):
        plt.fill_between(
            common_time,
            bottom,
            cumulative[i],
            color=COLORS[i],  
            alpha=0.5,          
            edgecolor="black",   
            linewidth=1
        )

        mid = len(common_time) // 2
        y_text = (bottom[mid] + cumulative[i][mid]) / 2
        plt.text(
            common_time[mid],
            y_text,
            labels[i],
            ha="center",
            va="center",
            fontsize=17,
            color="black"
        )

        bottom = cumulative[i]

    plt.xlabel("Time (ms)")
    plt.ylabel("Credit Allocation Ratio",labelpad=25)
    plt.xlim(0, 1.0)
    plt.ylim(0, 1.05)
    ax = plt.gca()
    ax.yaxis.set_label_coords(-0.15, 0.42)
    ax.tick_params(axis="x", pad=6)
    plt.grid(True, linestyle="--", alpha=0.5)

    plt.savefig(save_path, dpi=300, bbox_inches="tight")
    plt.close()

    print(f"✅ Saved to {save_path}")

# =========================
# 主入口
# =========================
if __name__ == "__main__":
    save_path = os.path.join(
        figure_root, "18-dt-limitation-credit-allocation.png"
    )
    plot_stacked_inflight(metrics_root, metric, target_files, save_path)
