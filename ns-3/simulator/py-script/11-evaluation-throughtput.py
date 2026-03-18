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

def load_throughput_data():
    """
    根据表格手动提取的吞吐量数据
    x: VC 数量
    y: 吞吐量 PPS (Gbps)
    """
    data = {
        "16": 99.5, "32": 99.4,
        "64": 99.5, "128": 99.4,
        "256": 99.3, "512": 99.4,
        "1024": 99.2, "2048": 99.2,
        "4096": 99.2,  "8192": 99.1,
        "16384": 98.9, "32768": 99.0,
        "65536": 98.8
    }
    return list(data.keys()), list(data.values())

def plot_throughput(save_path):
    # 全局字体 17
    plt.rcParams.update({'font.size': 17})
    
    x_data, y_data = load_throughput_data()

    # 画布尺寸与频率图保持一致
    fig, ax = plt.subplots(figsize=(4.5, 3.5))
    
    # 使用与脚本一致的红色钻石标记
    ax.plot(x_data, y_data, marker='D', color='#9467bd', lw=2, ms=6, label='Throughput')
    
    # 坐标轴标签
    ax.set_xlabel('The Number of VCs', fontsize=17, labelpad=12)
    ax.set_ylabel('Throughput (Gbps)', fontsize=17, labelpad=12)
    
    # 设置纵坐标范围，留出一点顶部空间
    ax.set_ylim(95, 100)

    # 刻度设置：16, 64, 256, 1K, 4K, 16K, 64K (4倍步长)
    xticks_subset = x_data[::2]
    ax.set_xticks(xticks_subset)
    ax.set_xticklabels(xticks_subset, 
                       fontsize=17, rotation=45)

    ax.grid(True, linestyle='--', alpha=0.5)
    
    # 确保保存路径存在
    os.makedirs(os.path.dirname(save_path), exist_ok=True)
    plt.tight_layout()
    # 适配旋转刻度
    plt.subplots_adjust(bottom=0.25)
    
    plt.savefig(save_path, dpi=300, bbox_inches='tight')
    print(f"Successfully saved throughput plot to {save_path}")
    plt.show()

# 运行路径
SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

save_path_tp = REPO_ROOT + '/ns-3/simulator/result/graph/11-evaluation-throughtput.png'
plot_throughput(save_path_tp)