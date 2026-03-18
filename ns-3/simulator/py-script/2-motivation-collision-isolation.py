import matplotlib.pyplot as plt
import os


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

data_dir = os.path.join(REPO_ROOT, "ns-3/simulator/result/data/2-motivation")
output_dir = os.path.join(REPO_ROOT, "ns-3/simulator/result/graph")

os.makedirs(output_dir, exist_ok=True)

# ================= 配置 =================
holb_cases = [
    ("ns", "w/o HoLB"),
    ("alone", "w HoLB"),
]

algorithms = [
    ("standalone CBFC", "cbfc", '#32936F', 3),
    ("standalone PFC",  "pfc",  '#855A8A', 2),
]

flow_ids = [10001, 10000]

# ================= 全局风格 =================
plt.rcParams.update({
    'font.size': 17,
    'axes.labelsize': 17,
    'pdf.fonttype': 42,
    'ps.fonttype': 42
})

# ================= 按 HoLB 画两幅图 =================
for sim, holb_label in holb_cases:

    fig, axes = plt.subplots(
        nrows=2,
        ncols=1,
        figsize=(4, 3),
        sharex=True
    )

    for ax, flow_id in zip(axes, flow_ids):

        for alg_label, alg_name, color, zorder in algorithms:

            # ===== 新文件命名 =====
            file_path = os.path.join(
                data_dir,
                f"sendingrate_{alg_name}_{sim}.txt"
            )

            times, rates = [], []

            if os.path.exists(file_path):
                with open(file_path, 'r') as f:
                    for line in f:
                        data = line.strip().split()

                        if len(data) < 3:
                            continue

                        if int(data[0]) != flow_id:
                            continue

                        rate = float(data[1])
                        time = float(data[2]) / 1e6  # ns → ms

                        if time >= 0:
                            times.append(time)
                            rates.append(rate)

            else:
                print(f"[WARN] file not found: {file_path}")

            # ===== 自动补 0 =====
            if times:

                last_time = times[-1]

                if rates[-1] > 0:

                    if len(times) >= 2:
                        avg_interval = (times[-1] - times[0]) / (len(times) - 1)
                        new_time = last_time + avg_interval
                    else:
                        new_time = last_time + 0.1

                    times.append(new_time)
                    rates.append(0.0)

                ax.plot(
                    times,
                    rates,
                    linewidth=2,
                    color=color,
                    label=alg_label,
                    zorder=zorder
                )

        # ===== 坐标轴设置 =====
        ax.set_xlim(0, 7)
        ax.set_ylim(0, 105)
        ax.set_yticks([0, 50, 100])

        ax.grid(True, linestyle="--", alpha=0.3)
        ax.tick_params(axis='both', which='major', pad=3)

        # ===== 子图标注 =====
        label = "Congested" if flow_id == flow_ids[0] else "Victim"

        ax.text(
            0.984,
            0.035,
            label,
            transform=ax.transAxes,
            ha="right",
            va="bottom",
            fontsize=15,
            bbox=dict(
                boxstyle="square,pad=0.2",
                edgecolor="black",
                facecolor="none",
                linewidth=0.5
            )
        )

    # ===== 图例 =====
    axes[0].legend(
        loc="upper right",
        frameon=False,
        ncol=1,
        handlelength=1.0,
        columnspacing=0.6,
        labelspacing=0.1,
        fontsize=17,
        borderaxespad=0.1
    )

    # ===== 坐标轴标签 =====
    axes[1].set_xlabel("Time(ms)", labelpad=1)

    fig.text(
        -0.03,
        0.5,
        "Throughput (Gbps)",
        va="center",
        rotation="vertical",
        fontsize=17
    )

    # ===== 布局 =====
    plt.subplots_adjust(
        left=0.18,
        right=0.96,
        top=0.96,
        bottom=0.12,
        hspace=0.12
    )

    # ===== 保存 =====
    out_path = os.path.join(
        output_dir,
        f"2-motivation-sendingrate_cbfc_vs_pfc_{sim}.png"
    )

    plt.savefig(out_path, dpi=300, bbox_inches="tight")
    plt.close()

    print(f"Saved: {out_path}")