import os
import matplotlib.pyplot as plt

def format_ticks(val_str):
    """处理横坐标刻度，1024及以上转为 K"""
    try:
        val = int(val_str)
        if val >= 1024:
            return f'{val // 1024}K'
        return str(val)
    except:
        return val_str

def load_freq_data_from_image():
    """
    根据图片标注手动提取的数据点 
    x: VC 数量 (16, 32, 64, ..., 64K)
    y: 对应的 Frequency (MHz)
    """
    data = {
        "16": 414, "32": 410, "64": 411, "128": 416,
        "256": 422, "512": 410, "1024": 414, "2048": 413,
        "4096": 406, "8192": 349, "16384": 338
        ,"32768": 336,"65536": 287
    }
    return list(data.keys()), list(data.values())

def plot_top_frequency(save_path):
    # 全局字体 17
    plt.rcParams.update({'font.size': 17})
    
    # 获取提取的数据 
    x_data, y_data = load_freq_data_from_image()

    # 微调画布高度以适应旋转刻度
    fig, ax = plt.subplots(figsize=(4.5, 3.5))
    
    # 按照脚本要求使用 tab:red 和钻石型标记 (D)
    ax.plot(x_data, y_data, marker='D', color='tab:red', lw=2, ms=6, label='Avg Freq')
    
    # 标签设置
    ax.set_xlabel('The Number of VCs', fontsize=17, labelpad=12)
    ax.set_ylabel('Frequency (MHz)', fontsize=17, labelpad=12)
    ax.set_ylim(250, 500) # 根据新数据点 468 调整了上限
    
    # 刻度设置：16, 64, 256, 1K... (即每隔两个点取一个，保持4倍步长)
    xticks_subset = x_data[::2]
    ax.set_xticks(xticks_subset)
    ax.set_xticklabels(xticks_subset, 
                       fontsize=17, rotation=45)

    ax.grid(True, linestyle='--', alpha=0.5)
    
    # 保存设置
    os.makedirs(os.path.dirname(save_path), exist_ok=True)
    plt.tight_layout()
    plt.subplots_adjust(bottom=0.25)
    plt.savefig(save_path, dpi=300, bbox_inches='tight')
    print(f"Successfully saved to {save_path}")
    plt.show()

# 运行
SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

save_path = REPO_ROOT + 'ns-3/simulator/result/graph/11-evaluation-maximum_frequency.png'
plot_top_frequency(save_path)