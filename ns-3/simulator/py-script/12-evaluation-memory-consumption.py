import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

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
    bram_data = []
    i = 0
    MAX_VC = 65536
    
    while i < len(df):
        val_raw = str(df.iloc[i, 0])
        if val_raw.replace('.', '', 1).isdigit():
            val_num = float(val_raw)
            if val_num > MAX_VC:
                break

            x_axis.append(val_raw)
            bram_val = float(df.iloc[i + 3, 2]) * 4.5 * 1.024 - 60  # BRAM 转 KB
            bram_data.append(bram_val)
            i += 4
        else:
            i += 1
    return x_axis, bram_data

def plot_bram_log_with_slanted_arrow(file_path, save_path):
    # 1. 读取数据
    df = pd.read_excel(file_path, header=None)
    x_data, bram_data = load_data(df)

    # 2. 创建画布
    fig, ax = plt.subplots(figsize=(7, 3.5))

    # --- 绘制 BRAM 曲线 ---
    ax.plot(x_data, bram_data, marker='^', ms=5, lw=1.5, color='#C00000', label='BRAM')

    # --- 对数坐标 ---
    ax.set_yscale('log')
    ax.yaxis.set_minor_locator(ticker.NullLocator())
    ax.set_ylabel(
        'Overall Memory\nConsumption (KB)', 
        rotation=90, 
        labelpad=24,    # 标题与轴线的水平间距
        y=0.4,         # 关键点：y=0.5 是中间，调小该值标题会向下移动
        va='center',    # 垂直对齐方式设为居中，便于通过 y 精确控制
        ha='center'
    )
    ax.set_ylim(50, 1600)
    ax.set_yticks([50, 100, 200, 400, 800, 1600])
    ax.get_yaxis().set_major_formatter(plt.ScalarFormatter())

    # --- 横坐标 ---
    xticks_subset = x_data[::1]
    ax.set_xticks(xticks_subset)
    ax.set_xticklabels(xticks_subset, rotation=45)
    ax.set_xlabel('The Number of VCs')

    # --- 网格：仅纵向 ---
    ax.grid(axis='y', linestyle='--', alpha=0.4)

    # --- 在横坐标 16384 的位置加斜向箭头标注 ---
    target_x = '16384'
    if target_x in x_data:
        idx = x_data.index(target_x)
        y_val = bram_data[idx]

        # 箭头标注：尾部是文本，箭头指向曲线上的点
        ax.annotate(
            f'{y_val:.0f} KB',
            xy=(target_x, y_val),                   # 箭头指向点
            xytext=(str(int(int(target_x)/2)), y_val*2),  # 文本位置（左上）
            arrowprops=dict(arrowstyle='->', color='black', lw=1.5),
            fontsize=14,
            ha='center',
            va='bottom'
        )

        # 竖线保留
        ax.axvline(x=target_x, color='black', linestyle='--', linewidth=1.5)

    # --- 保存与展示 ---
    os.makedirs(os.path.dirname(save_path), exist_ok=True)
    plt.subplots_adjust(
        left=0.2,    # 左边：给竖直标题留空间
        right=0.96,
        bottom=0.32,
        top=0.9
    )
    plt.savefig(save_path, dpi=300)
    plt.show()


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

file_path = os.path.join(REPO_ROOT, "ns-3/simulator/result/fpga-data/infiniflow_utilization.xlsx")

save_path = REPO_ROOT + 'ns-3/simulator/result/graph/12-evaluation-memory-consumption.png'

plot_bram_log_with_slanted_arrow(file_path, save_path)
