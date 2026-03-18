import matplotlib.pyplot as plt
import os

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

data_dir = os.path.join(REPO_ROOT, "ns-3/simulator/result/data/2-motivation")
output_dir = os.path.join(REPO_ROOT, "ns-3/simulator/result/graph")

os.makedirs(output_dir, exist_ok=True)

# ================= 配置 =================
simulations = [
    ("dcqcn", "DCQCN + PFC"),
    ("hpcc", "HPCC + PFC")
]
    # '#32936F',  # 绿
    # '#855A8A', #紫
flow_ids = [10001, 10000]
colors = ['#32936F', '#855A8A']

# ================= 全局风格 =================
plt.rcParams.update({
    'font.size': 17,
    'axes.labelsize': 17,
    'pdf.fonttype': 42,
    'ps.fonttype': 42
})

# ================= 创建画布 =================
# figsize (宽, 高)，高度适当缩小使结构更紧凑
fig, axes = plt.subplots(
    nrows=2, ncols=1, 
    figsize=(4, 3), 
    sharex=True
)

# ================= 绘图 =================
for i, (ax, flow_id) in enumerate(zip(axes, flow_ids)):
    for idx, (sim, label) in enumerate(simulations):
        file_path = os.path.join(data_dir, f"sendingrate_{sim}.txt")
        
        times, rates = [], []
        if os.path.exists(file_path):
            with open(file_path, 'r') as f:
                for line in f:
                    data = line.strip().split()
                    if len(data) < 3: continue
                    if int(data[0]) != flow_id: continue
                    
                    rate = float(data[1])
                    time = float(data[2]) / 1e6  # ns -> ms
                    if 0 <= time:  # 移除时间上限，读取所有时间
                        times.append(time)
                        rates.append(rate)
        else:
            print(f"[WARN] sendingrate file not found: {file_path}")
        
        if times:
            # 获取最后一个时间点
            last_time = times[-1]
            
            # 如果最后一个速率不为0，添加一个降到0的点
            if rates[-1] > 0 and last_time < 6.0:  # 只在6ms前才补全
                # 在最后一个时间点后面增加一个小间隔
                if len(times) >= 2:
                    # 使用平均时间间隔作为增量
                    avg_interval = (times[-1] - times[0]) / (len(times) - 1) if len(times) > 1 else 0.1
                    new_time = min(last_time + avg_interval, 6.0)
                else:
                    new_time = min(last_time + 0.1, 6.0)
                
                times.append(new_time)
                rates.append(0.0)
            
            ax.plot(times, rates, linewidth=2, color=colors[idx], label=label)

    # 统一坐标轴设置
    ax.set_xlim(0, 7)
    ax.set_ylim(0, 105) # 两个图统一为 105
    ax.set_yticks([0, 50, 100])
    ax.grid(True, linestyle="--", alpha=0.3)
    
    # 移除刻度向内的空白（可选，让图更紧凑）
    ax.tick_params(axis='both', which='major', pad=3)
    
    # ===== 子图内部标注（右下角）=====
    if flow_id == flow_ids[0]:
        label = "Congested"
    else:
        label = "Victim"

    ax.text(
        0.984, 0.035, label,
        transform=ax.transAxes,
        ha="right", va="bottom",
        fontsize=15,
        bbox=dict(
            boxstyle="square,pad=0.2",
            edgecolor="black",
            facecolor="none",
            linewidth=0.5
        )
    )

# ================= 布局优化 =================

# 1. 图例：放入第一个图内部右上角，尽量贴边
axes[0].legend(
    loc="upper right",
    frameon=False,
    ncol=1,
    handlelength=1.0,    # 缩短图例线长度
    columnspacing=0.6,   # 缩短列间距
    labelspacing=0.1,    # 缩短行间距
    fontsize=17,        # 稍微调小字号防止遮挡数据
    borderaxespad=0.1    # 尽量靠近边框
)

# 2. 坐标轴标签
axes[1].set_xlabel("Time(ms)", labelpad=1)

# 手动设置公共 Y 轴标签
# x=0.01 是标题位置，left=0.18 是给数字留出的空间
fig.text(-0.03, 0.5, 'Throughput (Gbps)', va='center', rotation='vertical', fontsize=17)

# 3. 紧凑布局核心：减小 hspace
# left=0.18 保证标题不挡数字；hspace=0.12 让上下图贴近
plt.subplots_adjust(left=0.18, right=0.96, top=0.96, bottom=0.12, hspace=0.12)

# ================= 保存 =================
out_path = os.path.join(output_dir, "2-motivation-sendingrate_dcqcn_hpcc.png")
plt.savefig(out_path, dpi=300,bbox_inches='tight')
plt.close()

print(f"Saved: {out_path}")