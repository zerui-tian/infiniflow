import matplotlib.pyplot as plt
import os


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

data_dir = os.path.join(REPO_ROOT, "ns-3/simulator/result/data/2-motivation")
output_dir = os.path.join(REPO_ROOT, "ns-3/simulator/result/graph")

os.makedirs(output_dir, exist_ok=True)

data_dirs = [
    (REPO_ROOT + "/ns-3/simulator/result/data/13-evaluation-ablation-study",
     "sendingrate_infiniflow.txt",
     "InfiniFlow"),
    (REPO_ROOT + "/ns-3/simulator/result/data/13-evaluation-ablation-study",
     "sendingrate_notap.txt",
     "w/o Locking"),
    (REPO_ROOT + "/ns-3/simulator/result/data/13-evaluation-ablation-study",
     "sendingrate_nothreshold.txt",
     "w/o BUCP"),
]

# ================= 原有配置（保持不变） =================
simulations = [
    ("nocc", "w/o CC w HoLB"),
]
flow_ids = [10001, 10000]
colors = ['#FFBF00', '#C00000', '#2274A5', ]
ZORDER_MAP = {
    '#C00000': 1,  # 红色：最底
    '#FFBF00': 4,  # 黄色
    '#2274A5': 3,  # 蓝色
    # '#32936F': 4,  # 绿色：最顶
}

# ================= 全局风格 =================
plt.rcParams.update({
    'font.size': 17,
    'axes.labelsize': 17,
    'pdf.fonttype': 42,
    'ps.fonttype': 42
})

# ================= 创建画布（不变） =================
fig, axes = plt.subplots(
    nrows=2,
    ncols=1,
    figsize=(4.5, 3),
    sharex=True
)

# ================= 绘图（结构完全不变，只是多读一个文件） =================
for i, (ax, flow_id) in enumerate(zip(axes, flow_ids)):
    for d_idx, (data_dir, filename, exp_label) in enumerate(data_dirs):
        for _, _ in simulations:
            file_path = os.path.join(data_dir, filename)

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
                        time = float(data[2]) / 1e6  # ns -> ms
                        if 0 <= time <= 6:  # 读取0-6ms的数据
                            times.append(time)
                            rates.append(rate)
            
            color = colors[d_idx]
            if times:
                # 获取最后一个时间点
                last_time = times[-1]
                
                # 如果最后一个速率不为0，添加一个降到0的点
                if rates[-1] > 0 and last_time < 6.0:  # 只在6ms前才补全
                    # 在最后一个时间点后面增加0.05ms的小间隔
                    new_time = min(last_time + 0.05, 6.0)
                    times.append(new_time)
                    rates.append(0.0)
                
                ax.plot(
                    times,
                    rates,
                    linewidth=2,
                    color=color,
                    label=exp_label,
                    zorder=ZORDER_MAP[color]
                )
    
    # ===== 子图右下角标注 =====
    if flow_id == flow_ids[0]:
        tag = "Congested"
    else:
        tag = "Victim"

    ax.text(
        0.984, 0.035, tag,
        transform=ax.transAxes,
        ha="right", va="bottom",
        fontsize=15,
        bbox=dict(
            boxstyle="square,pad=0.2",
            edgecolor="black",
            facecolor="white",
            linewidth=0.5
        )
    )

    # ---------- 坐标轴样式（保持不变） ----------
    ax.set_xlim(0, 7)
    ax.set_ylim(0, 105)
    ax.set_yticks([0, 50, 100])
    ax.grid(True, linestyle="--", alpha=0.3)
    ax.tick_params(axis='both', which='major', pad=3)

# ================= 图例（不改布局，只做防护） =================
handles, labels = axes[0].get_legend_handles_labels()

if handles:
    # ===== legend 锚定顺序 =====
    first_label = "CBFC w/o HoLB"
    second_label = "w/o Locking"

    # 统一用列表操作，保证稳定
    def move_to_position(handles, labels, label, pos):
        if label in labels:
            idx = labels.index(label)
            h = handles.pop(idx)
            l = labels.pop(idx)
            handles.insert(pos, h)
            labels.insert(pos, l)

    # 先放第一个，再放第二个（顺序不能反）
    move_to_position(handles, labels, first_label, 0)
    move_to_position(handles, labels, second_label, 1)

    axes[0].legend(
        handles,
        labels,
        loc="upper right",
        bbox_to_anchor=(1.03, 1.15),  # x=1保持右对齐，y>1向上偏移
        frameon=False,
        ncol=1,
        handlelength=1.0,
        columnspacing=0.6,
        labelspacing=0.1,
        fontsize=17,
        markerfirst=False
    )

# ================= 坐标轴标签（不变） =================
axes[1].set_xlabel("Time (ms)", labelpad=1)
fig.text(
    0,
    0.5,
    'Throughput (Gbps)',
    va='center',
    rotation='vertical',
    fontsize=17
)

# ================= 紧凑布局（不变） =================
plt.subplots_adjust(
    left=0.18,
    right=0.96,
    top=0.96,
    bottom=0.12,
    hspace=0.12
)

# ================= 保存 =================
out_path = os.path.join(output_dir, "13-evaluation-ablation-study.png")
plt.savefig(out_path, dpi=300, bbox_inches='tight')
plt.close()

print(f"Saved: {out_path}")