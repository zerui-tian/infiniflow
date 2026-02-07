import os
import pandas as pd
import matplotlib.pyplot as plt
import sys

script_dir = os.getcwd()

base_dir = sys.argv[1] + "/ns-3/simulator/ns-3.39/examples/infiniflow/1-control-dynamics/monitor_output"
save_dir = sys.argv[1] + "/ns-3/simulator/ns-3.39/examples/infiniflow/_result"
os.makedirs(save_dir, exist_ok=True)

COLORS = [
    '#2274A5',
    '#32936F',
    '#FFBF00',
    '#C00000',
]

plt.rcParams.update({
    "font.size": 17,
    "axes.titlesize": 17,
    "axes.labelsize": 17,
    "xtick.labelsize": 17,
    "ytick.labelsize": 17,
    "legend.fontsize": 17,
})

def plot_flowthroughput(
    file_path,
    save_path,
    target_sid,
    target_rid,
    flow_ids
):

    df = pd.read_csv(
        file_path,
        sep=r"\s+",
        names=["sid", "rid", "flow_id", "throughput_Gbps", "time_ns"],
    )

    df = df[(df["sid"] == target_sid) & (df["rid"] == target_rid)]

    if df.empty:
        print("[skip] no sid/rid data matched")
        return

    df["time_ms"] = df["time_ns"] / 1e3
    flow_ids = sorted(flow_ids)

    plt.figure(figsize=(4.5, 3))

    for idx, fid in enumerate(flow_ids):
        flow_df = df[df["flow_id"] == fid]
        if flow_df.empty:
            continue

        plt.plot(
            flow_df["time_ms"],
            flow_df["throughput_Gbps"],
            linewidth=2,
            color=COLORS[idx],
            label=f"flow {idx + 1}"
        )

    plt.xlabel("Time (us)")
    plt.ylabel("Throughput (Gbps)")
    plt.ylim(bottom=0)
    plt.xlim(0, 500)
    for y in [10, 70]:
        plt.axhline(
            y=y,
            color='black',
            linestyle='--',
            linewidth=1
        )

    plt.text(
        x=250,
        y=10 + 1, 
        s="10 Gbps",
        color='black',
        fontsize=14,
        ha='center',
        va='bottom' 
    )

    plt.text(
        x=250,
        y=70 - 5, 
        s="70 Gbps",
        color='black',
        fontsize=14,
        ha='center', 
        va='top'
    )


    plt.grid(True, linestyle="--", alpha=0.6)
    
    plt.legend(
        loc='upper center',
        bbox_to_anchor=(0.5, 1),
        frameon=False,
        ncol=2,
        borderaxespad=0.1,
        columnspacing=0.6,
        handletextpad=0.3,
        labelspacing=0.12
    )

    plt.savefig(save_path, dpi=300, bbox_inches="tight")
    plt.close()

    print("[OK] figure saved:", save_path)


file_path = os.path.join(base_dir, "flowthroughput.txt")

flow_ids = [10000, 10001, 10002, 10003]

save_path = os.path.join(save_dir, "InfiniFlow_sending_rate.png")

plot_flowthroughput(
    file_path=file_path,
    save_path=save_path,
    target_sid=8,
    target_rid=9,
    flow_ids=flow_ids
)

