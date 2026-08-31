import os
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib as mpl
mpl.rcParams["pdf.fonttype"] = 42
mpl.rcParams["ps.fonttype"] = 42

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))
save_dir = os.path.join(REPO_ROOT, "ns-3/simulator/result/graph")
os.makedirs(save_dir, exist_ok=True)

# ================= 颜色配置 =================
COLORS = [
    '#7D7C78',
    '#7D7C78',
    "#7D7C78",
    '#2274A5',
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

    # ======= 时间单位转换：从 ns 转换为 ms =======
    df["time_ms"] = df["time_ns"] / 1e6  # 修改为 ms

    # flow_id 排序，保证颜色稳定
    flow_ids = sorted(flow_ids)

    plt.figure(figsize=(4, 3))

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
    plt.xlabel("Time (ms)")  # 修改为 ms
    plt.xlim(0, 1)  # 1000 μs = 1 ms
    plt.ylabel("Throughput (Gbps)")
    plt.ylim(bottom=0)
    # 设置横坐标只显示首尾值
    plt.xticks([0, 1])
    
    # ===== 添加水平参考线 =====
    for y in [10, 70]:
        plt.axhline(
            y=y,
            color='black',
            linestyle='--',
            linewidth=1
        )

    # ===== 添加文本标注（移到虚线中间上方）=====
    plt.text(
        x=0.5,           # 横向中间位置（时间轴是 0~1 ms）
        y=10 + 3,        # 10Gbps 虚线稍上方
        s="10 Gbps",
        color='black',
        fontsize=18,
        ha='center',     # 水平居中
        va='bottom'      # 文字在虚线下方
    )

    plt.text(
        x=0.5,           # 横向中间位置
        y=70 + 3,        # 70Gbps 虚线稍上方
        s="70 Gbps",
        color='black',
        fontsize=18,
        ha='center',     # 水平居中
        va='bottom'      # 文字在虚线上方
    )

    plt.grid(True, linestyle="--", alpha=0.6)
    # plt.gca().yaxis.set_visible(False)  # 隐藏 Y 轴的刻度和标签
    # 移除图例（不调用 plt.legend()）
    # 图例已从坐标系中移除

    plt.savefig(save_path, dpi=300, bbox_inches="tight")
    plt.close()

    print("[OK] 保存图像:", save_path)


# ================================
# 3. 主流程
# ================================
file_path = os.path.join(REPO_ROOT, "ns-3/simulator/result/data/19-sensitivity/flowthroughput_2_4.txt")

flow_ids = [10000, 10001, 10002, 10003]

save_path = os.path.join(save_dir, "19-sensitivity-flow-throughput_small.png")

plot_flowthroughput(
    file_path=file_path,
    save_path=save_path,
    target_sid=8,
    target_rid=9,
    flow_ids=flow_ids
)

print("\n✅ Throughput draw finish")