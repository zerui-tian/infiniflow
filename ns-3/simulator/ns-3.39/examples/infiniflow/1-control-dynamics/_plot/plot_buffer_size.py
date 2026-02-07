#!/usr/bin/env python3
import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys


metrics_root = (
    sys.argv[1] + "/ns-3/simulator/ns-3.39/examples/infiniflow/1-control-dynamics/monitor_output/metrics_output/inflight"
)
figure_root = (
    sys.argv[1] + "/ns-3/simulator/ns-3.39/examples/infiniflow/_result"
)
os.makedirs(figure_root, exist_ok=True)

metric = "inflight"

FILE_FLOW_MAP = {
    "node8_port5_q2.csv": "flow1",
    "node8_port5_q3.csv": "flow2",
    "node8_port5_q4.csv": "flow3",
    "node8_port5_q5.csv": "flow4",
}

FLOW_ORDER = ["flow1", "flow2", "flow3", "flow4"]

BIN_SIZE_NS = 1000          # 1000 ns = 1 us
BUFFER_SIZE = 2 * 100 * 3000 * 2 / 8   # kB
FLOW_COLORS = {
    "flow1": "#2274A5",
    "flow2": "#32936F",
    "flow3": "#FFBF00",
    "flow4": "#C00000",
}

def resample_by_time_bin(df, value_col, bin_size_ns):
    df = df[(df["Time"] >= 0) & (df["Time"] <= 1_000_000)].copy()
    df["bin"] = (df["Time"] // bin_size_ns).astype(int)

    grouped = df.groupby("bin")[value_col].mean()

    time_ns = (grouped.index + 0.5) * bin_size_ns
    time_ms = time_ns / 1e3

    return time_ms.to_numpy(), grouped.to_numpy()

def plot_stacked_inflight_by_flow(metric_root, save_path):

    plt.rcParams.update({
        "font.size": 17,
        "axes.labelsize": 17,
        "xtick.labelsize": 17,
        "ytick.labelsize": 17,
    })

    flow_values = {}
    common_time = None

    for file_name, flow_name in FILE_FLOW_MAP.items():
        file_path = os.path.join(metric_root, file_name)
        df = pd.read_csv(file_path)

        time_ms, values = resample_by_time_bin(
            df, value_col=metric, bin_size_ns=BIN_SIZE_NS
        )

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
        bottom = top


    plt.xlabel("Time (us)")
    plt.ylabel("Credit Allocation Ratio",labelpad=25)
    plt.xlim(0, 500)
    plt.ylim(0, 1.05)


    per_hop_ratio = 0.5  
    plt.axhline(
        y=per_hop_ratio,
        color='black',
        linestyle='--',
        linewidth=1
    )
    plt.text(
        x=250,  
        y=per_hop_ratio + 0.02,
        s="1 x Per-hop BDP",
        color='black',
        fontsize=14,
        ha='center',
        va='bottom'
    )

    ax = plt.gca()
    ax.yaxis.set_label_coords(-0.15, 0.42)
    ax.tick_params(axis="x", pad=6)
    plt.grid(True, linestyle="--", alpha=0.5)

    plt.legend(
        loc='upper center',
        bbox_to_anchor=(0.5, 1), 
        ncol=2, 
        frameon=False, 
        fontsize=17
    )

    plt.savefig(save_path, dpi=300, bbox_inches="tight")
    plt.close()

    print(f"✅ Saved to {save_path}")

if __name__ == "__main__":
    save_path = os.path.join(
        figure_root, "Infiniflow_inflight_stacked_buffer_usage.png"
    )
    plot_stacked_inflight_by_flow(metrics_root, save_path)
