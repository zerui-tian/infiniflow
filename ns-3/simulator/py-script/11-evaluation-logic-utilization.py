import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

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

def load_data(df):
    x_axis = []
    data_map = {'LUT': [], 'FF': [], 'LUTRAM': []}
    i = 0
    MAX_VC = 65536

    while i < len(df):
        val_raw = str(df.iloc[i, 0])
        if val_raw.replace('.', '', 1).isdigit():
            val_num = float(val_raw)
            if val_num > MAX_VC:
                break

            x_axis.append(val_raw)
            data_map['LUT'].append(float(df.iloc[i, 4]))
            data_map['LUTRAM'].append(float(df.iloc[i + 1, 4]))
            data_map['FF'].append(float(df.iloc[i + 2, 4]))
            i += 4
        else:
            i += 1

    return x_axis, data_map

def plot_fpga_logic_only(file_path, save_path):
    # 1. 读取数据
    xls = pd.ExcelFile(file_path)
    df = pd.read_excel(xls, sheet_name=0, header=None)
    x_data, d_map = load_data(df)

    # 2. 创建画布（单图）
    fig, ax = plt.subplots(figsize=(4.5, 3.5))

    ms = 5
    lw = 1.5

    # --- Logic Utilization ---
    l1, = ax.plot(x_data, d_map['LUT'], marker='o', ms=ms, lw=lw,
                  color='#2ca02c', label='LUT')
    l2, = ax.plot(x_data, d_map['FF'], marker='s', ms=ms, lw=lw,
                  color='#1f77b4', label='FF')
    l3, = ax.plot(x_data, d_map['LUTRAM'], marker='x', ms=ms, lw=lw,
                  color='#C00000', label='LUTRAM')

    ax.set_ylabel('Logic Utilization (%)')
    ax.set_ylim(0, 0.31)
    ax.set_yticks(np.arange(0, 0.31, 0.05))

    # X 轴刻度（隔一个取一个）
    xticks_subset = x_data[::2]
    ax.set_xticks(xticks_subset)
    ax.set_xticklabels(xticks_subset, rotation=45)

    ax.grid(True, linestyle='--', alpha=0.4)

    # 3. 横坐标标题
    ax.set_xlabel('The Number of VCs')

    # 4. 图例（仅前三个）
    ax.legend(
        loc='upper center',
        bbox_to_anchor=(0.4, 1.3),   # 坐标系上方
        ncol=3,                       # 横向三列，更紧凑
        frameon=False,
        columnspacing=0.8,            # 列间距（默认 2.0）
        handletextpad=0.4,            # 线段-文字间距
        handlelength=1.4              # 线段长度
    )

    # 5. 保存与展示
    os.makedirs(os.path.dirname(save_path), exist_ok=True)
    plt.subplots_adjust(
        left=0.24,
        right=0.96,
        bottom=0.32,
        top=0.88
    )

    plt.savefig(save_path, dpi=300)
    plt.show()

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

file_path = os.path.join(REPO_ROOT, "ns-3/simulator/result/fpga-data/infiniflow_utilization.xlsx")

save_path = REPO_ROOT + '/ns-3/simulator/result/graph/11-evaluation-logic-utilization.png'

plot_fpga_logic_only(file_path, save_path)
