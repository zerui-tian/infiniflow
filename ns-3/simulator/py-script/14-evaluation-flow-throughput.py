import os
import pandas as pd
import matplotlib.pyplot as plt


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))
save_dir = os.path.join(REPO_ROOT, "ns-3/simulator/result/graph")
os.makedirs(save_dir, exist_ok=True)

# ================= 颜色配置（与 sendingrate 完全一致） =================
COLORS = [
    '#2274A5',
    '#32936F',
    '#FFBF00',
    '#C00000',
]

# ================= 字体配置 =================
plt.rcParams.update({
    "font.size": 17,
    "axes.titlesize": 17,
    "axes.labelsize": 17,
    "xtick.labelsize": 17,
    "ytick.labelsize": 17,
    "legend.fontsize": 17,
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

    # ======= 时间单位转换 =======
    df["time_ms"] = df["time_ns"] / 1e3

    # flow_id 排序，保证颜色稳定
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
            label=f"flow {idx + 1}"   # ✅ 统一命名
        )

    # ================= 坐标与风格 =================
    plt.xlabel("Time (μs)")
    plt.ylabel("Throughput (Gbps)")
    plt.ylim(bottom=0)
    plt.xlim(0, 500)
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
        x=250,           # 横向中间位置（时间轴是 0~500 us）
        y=10 + 1,        # 10Gbps 虚线稍上方
        s="10 Gbps",
        color='black',
        fontsize=14,
        ha='center',     # 水平居中
        va='bottom'      # 文字在虚线下方
    )

    plt.text(
        x=250,           # 横向中间位置
        y=70 - 5,        # 70Gbps 虚线稍下方
        s="70 Gbps",
        color='black',
        fontsize=14,
        ha='center',     # 水平居中
        va='top'         # 文字在虚线上方
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

    print("[OK] 保存图像:", save_path)


# ================================
# 3. 主流程
# ================================
file_path = os.path.join(REPO_ROOT, "ns-3/simulator/result/data/14-evaluation/flowthroughput.txt")

flow_ids = [10000, 10001, 10002, 10003]

save_path = os.path.join(save_dir, "14-evaluation-flow-throughput.png")

plot_flowthroughput(
    file_path=file_path,
    save_path=save_path,
    target_sid=8,
    target_rid=9,
    flow_ids=flow_ids
)

print("\n✅ Throughput draw finish")
