import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter

# =========================
# 全局字体配置
# =========================
plt.rcParams.update({
    "font.size": 17,
    "axes.titlesize": 17,
    "axes.labelsize": 17,
    "xtick.labelsize": 17,
    "ytick.labelsize": 17,
    "legend.fontsize": 17,
})

def load_x_axis(df):
    """
    只读取横坐标（VC 数量）
    """
    x_axis = []
    i = 0
    MAX_VC = 65536
    while i < len(df):
        val_raw = str(df.iloc[i, 0])
        if val_raw.replace('.', '', 1).isdigit():
            val_num = float(val_raw)
            if val_num > MAX_VC:
                break
            x_axis.append(val_raw)
            i += 4
        else:
            i += 1
    return x_axis

def plot_bram_cbfc_pfc(file_path, save_path):
    # 1. 读取数据
    df = pd.read_excel(file_path, header=None)
    x_data_str = load_x_axis(df)
    x_data = np.array([float(v) for v in x_data_str])  # 转 float 用于计算

    # 2. 曲线数据 (单位 KB -> MB)
    infiniflow = np.full_like(x_data, 64.0) / 1000        # MB
    cbfc = (18.75 * x_data) / 1000                        # MB
    pfc = (18.75 * 2 * x_data) / 1000                     # MB

    # 3. 创建画布
    fig, ax = plt.subplots(figsize=(4.5, 3.5))

    # --- 绘制三条曲线 ---
    ax.plot(x_data_str, infiniflow, color='#C00000', lw=2, label='InfiniFlow', linestyle='-',zorder=4)       # 实线
    ax.plot(x_data_str, cbfc, color='#1f77b4', lw=2, label='CBFC', linestyle='--', zorder=3)                  # 长虚线
    ax.plot(x_data_str, pfc, color='#f47F38', lw=2, label='PFC', linestyle='-.',zorder=2)                   # 点划线

    # --- 纵轴改为线性 ---
    ax.set_yscale('linear')
    ax.set_ylabel(
        'Packet Buffer\nRequirement (MB)',
        rotation=90,
        labelpad=22,
        y=0.4,
        va='center',
        ha='center'
    )

    # --- 设置纵轴范围和刻度 ---
    ax.set_ylim(-100, 2500)  # MB
    ax.set_yticks([0, 500, 1000, 1500, 2000, 2500])
    ax.get_yaxis().set_major_formatter(FuncFormatter(lambda y, _: f'{y:g}'))

    # --- 横坐标 ---
    ax.set_xticks(x_data_str[::2])
    ax.set_xticklabels(x_data_str[::2], rotation=45)
    ax.set_xlabel('The Number of VCs')

    # --- 网格（坐标系）放到最下层 ---
    ax.grid(axis='y', linestyle='--', alpha=0.4, zorder=1)


    # --- 在 VC=16384 位置加竖线和箭头（箭头指向 Infiniflow） ---
    # --- 在 VC=16384 位置加标注 ---
    target_x = 16384
    if str(target_x) in x_data_str:
        idx = x_data_str.index(str(target_x))
        y_val_kb = infiniflow[idx] * 1000  # 75
        y_val_mb = infiniflow[idx]         # 0.075

        # 计算位置：
        # idx 是 16384 的索引位
        # idx - 0.2 让文本向左移动，避开虚线
        # y=400 在 0-2500 的线性轴上属于“上方”位置，且不会飞出图外
        ax.annotate(
            f'{y_val_kb:2.0f} KB',
            xy=(str(target_x), y_val_mb),   # 箭头指向红线上的点
            xytext=(idx - 0.2, 600),        # 文本位置：索引减 0.2 (左移), y=400 (上移)
            arrowprops=dict(
                arrowstyle='->', 
                color='black', 
                lw=1.5,
                connectionstyle="arc3,rad=0.2" # rad为正，箭头会向左弯曲，避开竖线
            ),
            fontsize=14,
            ha='right',  # 文字右对齐，确保文字在 (idx-0.2) 的左边，不挡虚线
            va='bottom'
        )

        ax.axvline(x=str(target_x), color='black', linestyle='--', linewidth=1.5)

    # --- 图例：放在坐标系上方，一行三列 ---
    ax.legend(
        loc='upper center',
        bbox_to_anchor=(0.4, 1.3),
        ncol=3,
        frameon=False,
        columnspacing=0.8,
        handletextpad=0.4,
        handlelength=1.4
    )

    # --- 调整布局并保存 ---
    os.makedirs(os.path.dirname(save_path), exist_ok=True)
    plt.subplots_adjust(
        left=0.3,
        right=0.96,
        bottom=0.32,
        top=0.88
    )
    plt.savefig(save_path, dpi=300)
    plt.show()


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

file_path = os.path.join(REPO_ROOT, "ns-3/simulator/result/fpga-data/infiniflow_utilization.xlsx")

save_path = REPO_ROOT + '/ns-3/simulator/result/graph/11-evaluation-buffer-requirement.png'

plot_bram_cbfc_pfc(file_path, save_path)
