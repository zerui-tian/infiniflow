import os
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from matplotlib.legend_handler import HandlerTuple
import matplotlib as mpl
mpl.rcParams["pdf.fonttype"] = 42
mpl.rcParams["ps.fonttype"] = 42

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))
save_dir = os.path.join(REPO_ROOT, "ns-3/simulator/result/graph")
os.makedirs(save_dir, exist_ok=True)

# ================= 颜色配置 =================
COLORS = [
    '#7D7C78',  # other flows
    '#7D7C78',  # other flows
    '#7D7C78',  # other flows
    '#C00000',  # observed / target flow
]

# ================= 字体配置 =================
plt.rcParams.update({
    "font.size": 18,
    "axes.titlesize": 18,
    "axes.labelsize": 18,
    "xtick.labelsize": 18,
    "ytick.labelsize": 18,
    "legend.fontsize": 18,
})


# ================================
# 1. 生成独立图例 PDF
# ================================
def save_legend_pdf(save_dir):
    """生成独立图例 PDF：文字在前，线在后"""

    handles = [
        Line2D([0], [0], color='#2274A5', linewidth=2),
        Line2D([0], [0], color='#32936F', linewidth=2),
        Line2D([0], [0], color='#C00000', linewidth=2),
        Line2D([0], [0], color='#7D7C78', linewidth=2),
    ]

    labels = [
        'long-lived flow',
        '',
        '',
        '  other flows'
    ]

    fig = plt.figure(figsize=(4.5, 0.6))

    fig.legend(
        handles=handles,
        labels=labels,
        loc='center',
        frameon=False,
        ncol=4,
        columnspacing=0.6,   # 三条彩色线之间的间距
        handletextpad=0.5,   # 文字和线之间的距离
        handlelength=1.5,     # 每一条线自己的长度
        borderaxespad=0.0,
        markerfirst=False     # 文字在前，线在后
    )

    legend_path = os.path.join(save_dir, "19-sensitivity-flow-throughput_legend.png")
    fig.savefig(legend_path, dpi=300, bbox_inches="tight", transparent=True)
    plt.close(fig)

    print("[OK] 保存图例:", legend_path)
    return legend_path


# ================================
# 2. 绘图函数
# ================================
def plot_flowthroughput(
    file_path,
    save_path,
    target_sid,
    target_rid,
    flow_ids
):
    """
    绘制指定 sid / rid 下多个 flow 的 throughput 曲线
    """

    df = pd.read_csv(
        file_path,
        sep=r"\s+",
        names=["sid", "rid", "flow_id", "throughput_Gbps", "time_ns"],
    )

    # ======= 筛选 sid / rid =======
    df = df[(df["sid"] == target_sid) & (df["rid"] == target_rid)]

    if df.empty:
        print("[跳过] 无匹配 sid/rid 数据")
        return

    # ======= 时间单位转换：ns -> ms =======
    df["time_ms"] = df["time_ns"] / 1e6

    # flow_id 排序，保证颜色稳定
    flow_ids = sorted(flow_ids)

    plt.figure(figsize=(3.9, 3))

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

    # ================= 坐标与风格 =================
    plt.xlabel("Time (ms)")
    # plt.ylabel("Throughput (Gbps)")
    # plt.ylim(bottom=0)
    plt.xlim(0, 1)
    plt.ylim(bottom=0)
    # 横坐标只显示首尾值
    plt.xticks([0, 1])
    plt.yticks([0, 50, 100])
    # ===== 添加水平参考线 =====
    for y in [10, 70]:
        plt.axhline(
            y=y,
            color='black',
            linestyle='--',
            linewidth=1
        )

    # ===== 添加文本标注 =====
    plt.text(
        x=0.5,
        y=13,
        s="10 Gbps",
        color='black',
        fontsize=18,
        ha='center',
        va='bottom'
    )

    plt.text(
        x=0.5,
        y=73,
        s="70 Gbps",
        color='black',
        fontsize=18,
        ha='center',
        va='bottom'
    )

    plt.grid(True, linestyle="--", alpha=0.6)
    ax = plt.gca()
    ax.tick_params(axis="y", labelleft=False, left=False)
    # 主图不放图例，图例单独保存
    plt.savefig(save_path, dpi=300, bbox_inches="tight")
    plt.close()

    print("[OK] 保存图像:", save_path)


# ================================
# 3. 主流程
# ================================
file_path = os.path.join(
    REPO_ROOT,
    "ns-3/simulator/result/data/19-sensitivity/flowthroughput_28_36.txt"
)

flow_ids = [10000, 10001, 10002, 10003]

save_path = os.path.join(save_dir, "19-sensitivity-flow-throughput_large.png")

plot_flowthroughput(
    file_path=file_path,
    save_path=save_path,
    target_sid=8,
    target_rid=9,
    flow_ids=flow_ids
)

legend_path = save_legend_pdf(save_dir)

print("\n✅ Throughput draw finish")
print(f"✅ 图例已生成: {legend_path}")