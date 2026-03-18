#!/usr/bin/env python3
import os
import pandas as pd
import matplotlib.pyplot as plt

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

BASE_DIR = os.path.join(REPO_ROOT, "ns-3/simulator/result/data/19-validating-prototype")
NS3_DIR  = BASE_DIR 
FIG_DIR = os.path.join(REPO_ROOT, "ns-3/simulator/result/graph")
os.makedirs(FIG_DIR, exist_ok=True)

X_MAX_NS   = 350_000
TIME_SCALE = 1e3     # ns -> us

METRICS = {
    "Credit": (
        "credit/node2_port2_q2.csv", "credit", "Credit"
    ),
    "Threshold": (
        "threshold/node2_port2_q2.csv", "threshold", "Threshold"
    ),
    "Inflight": (
        "inflight/node2_port2_q2.csv", "inflight", "Inflight"
    ),
    "RemainBuf": (
        "remainBS/node3_port2_q2.csv", "remainBS", "RemainingBS"
    ),
}

FPGA_FILE = os.path.join(REPO_ROOT, "/ns-3/simulator/result/fpga-data/FPGA_flow_control_data.csv")
df_fpga = pd.read_csv(FPGA_FILE)
df_fpga = df_fpga[df_fpga["Time_ns"] <= X_MAX_NS]

plt.rcParams.update({
    "font.size": 17,
    "axes.labelsize": 17,
    "xtick.labelsize": 17,
    "ytick.labelsize": 17,
    "legend.fontsize": 17,
})

for title, (ns3_file, ns3_col, fpga_col) in METRICS.items():
    ns3_path = os.path.join(NS3_DIR, ns3_file)

    df_ns3 = pd.read_csv(ns3_path)
    df_ns3 = df_ns3[df_ns3["Time"] <= X_MAX_NS]

    plt.figure(figsize=(4.5, 3))

    plt.plot(
        df_ns3["Time"] / TIME_SCALE,
        df_ns3[ns3_col],
        label="NS-3",
        linewidth=2
    )

    if os.path.exists(FPGA_FILE):
        plt.plot(
            df_fpga["Time_ns"] / TIME_SCALE,
            df_fpga[fpga_col],
            label="FPGA",
            linewidth=1.5,
            linestyle="--"
        )

    plt.xlabel("Time (μs)")
    plt.ylabel(title + " (Packets)")
    plt.yticks([20, 40, 60])
    plt.ylim(0,68)
    
    if title in ("Credit", "RemainBuf"):
        legend_loc = "lower right"
    else:
        legend_loc = "upper right"

    plt.legend(
        loc=legend_loc,
        frameon=False
    )

    plt.grid(True, linestyle=":", linewidth=0.8)
    plt.subplots_adjust(
        left=0.16,
        right=0.98,
        bottom=0.20,
        top=0.95
    )

    out_path = os.path.join(FIG_DIR, f"19-validating-portotype-{title}.png")
    plt.savefig(out_path, dpi=300)

    plt.close()

print("✅ Saved!")