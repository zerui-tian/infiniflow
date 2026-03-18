#!/usr/bin/env python3
import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

metrics_root = os.path.join(REPO_ROOT, "ns-3/simulator/result/data/14-evaluation/inflight")

figure_root = os.path.join(REPO_ROOT, "ns-3/simulator/result/graph")
os.makedirs(figure_root, exist_ok=True)

metric = "inflight"

# ==========================================================
# 【关键】CSV 文件 → Flow 的显式映射关系
# ==========================================================
# ⚠️ 这里是“语义锚点”，论文/实验必须与之一致
FILE_FLOW_MAP = {
    "node8_port5_q2.csv": "flow1",
    "node8_port5_q3.csv": "flow2",
    "node8_port5_q4.csv": "flow3",
    "node8_port5_q5.csv": "flow4",
}

# 按 flow 顺序绘图（不会依赖 dict 顺序）
FLOW_ORDER = ["flow1", "flow2", "flow3", "flow4"]

# =========================
# 参数
# =========================
BIN_SIZE_NS = 1000          # 1000 ns = 1 us
BUFFER_SIZE = 2 * 100 * 3000 * 2 / 8   # kB
# =========================
# 颜色（flow 顺序）
# =========================
FLOW_COLORS = {
    "flow1": "#2274A5",
    "flow2": "#32936F",
    "flow3": "#FFBF00",
    "flow4": "#C00000",
}

# =========================
# 重采样函数
# =========================
def resample_by_time_bin(df, value_col, bin_size_ns):
    df = df[(df["Time"] >= 0) & (df["Time"] <= 1_000_000)].copy()
    df["bin"] = (df["Time"] // bin_size_ns).astype(int)

    grouped = df.groupby("bin")[value_col].mean()

    time_ns = (grouped.index + 0.5) * bin_size_ns
    time_ms = time_ns / 1e3

    return time_ms.to_numpy(), grouped.to_numpy()

# =========================
# 绘图函数（按 flow 堆叠）
# =========================
def plot_stacked_inflight_by_flow(metric_root, save_path):

    plt.rcParams.update({
        "font.size": 17,
        "axes.labelsize": 17,
        "xtick.labelsize": 17,
        "ytick.labelsize": 17,
    })

    flow_values = {}
    common_time = None

    # ---------- 读取 + 重采样 ----------
    for file_name, flow_name in FILE_FLOW_MAP.items():
        file_path = os.path.join(metric_root, file_name)
        df = pd.read_csv(file_path)

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
            for k in flow_values:
                flow_values[k] = flow_values[k][:min_len]
            values = values[:min_len]

        flow_values[flow_name] = values

    # ---------- 堆叠 ----------
    plt.figure(figsize=(4.5, 3))

    bottom = np.zeros_like(common_time)

    for flow in FLOW_ORDER:
        vals = flow_values[flow]
        top = bottom + vals

        plt.fill_between(
            common_time,
            bottom,
            top,
            color=FLOW_COLORS[flow],
            alpha=0.5,
            edgecolor="black",
            linewidth=1,
            label=flow
        )

        # # 区域内文字
        # mid = len(common_time) // 2
        # y_text = (bottom[mid] + top[mid]) / 2
        # plt.text(
        #     common_time[mid],
        #     y_text,
        #     flow,
        #     ha="center",
        #     va="center",
        #     fontsize=12,
        #     color="black"
        # )

        bottom = top

    # ---------- 样式 ----------
    plt.xlabel("Time (μs)")
    plt.ylabel("Credit Allocation Ratio",labelpad=25)
    plt.xlim(0, 500)
    plt.ylim(0, 1.05)

       # ---------- 添加中轴虚线及标注 ----------
    per_hop_ratio = 0.5  # 已经是归一化 buffer 使用比例
    plt.axhline(
        y=per_hop_ratio,
        color='black',
        linestyle='--',
        linewidth=1
    )
    plt.text(
        x=250,              # 横向中间位置（时间轴 0~500 us）
        y=per_hop_ratio + 0.02,  # 文字稍微偏上
        s="1 x Per-hop BDP",
        color='black',
        fontsize=14,
        ha='center',        # 水平居中
        va='bottom'         # 文字在虚线上方
    )

    ax = plt.gca()
    ax.yaxis.set_label_coords(-0.15, 0.42)
    ax.tick_params(axis="x", pad=6)
    plt.grid(True, linestyle="--", alpha=0.5)

        # ---------- 图例 ----------
    plt.legend(
        loc='upper center',   # 图上方居中
        bbox_to_anchor=(0.5, 1),  # 调整位置在图上方
        ncol=2,               # 2列
        frameon=False,        # 无边框
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
        figure_root, "14-evaluation-credit-allocation-ratio.png"
    )
    plot_stacked_inflight_by_flow(metrics_root, save_path)
