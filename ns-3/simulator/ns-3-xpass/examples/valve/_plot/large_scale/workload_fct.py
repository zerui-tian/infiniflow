import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib as mpl
from collections import defaultdict

# 设置全局字体和样式
plt.rcParams.update({
    'font.family': 'serif',
    'font.size': 12,
    'axes.labelsize': 14,
    'axes.titlesize': 16,
    'xtick.labelsize': 12,
    'ytick.labelsize': 12,
    'legend.fontsize': 12,
    'figure.figsize': (10, 6),
    'figure.dpi': 300
})

# 定义常量
ALGORITHMS = ["Valve"]  # 五种算法
CDF_CHOICES = ["websearch"]  # 三种场景
SELECTED_LOAD = 50  # 可以手动修改这个值(30, 50, 80)

SIZE_BINS = [
    (0, 500 * 1024),         # 0-500KB (不包含500KB)
    (500 * 1024, 5 * 1024 * 1024),  # 500KB-5MB (两边都包含5MB)
    (5 * 1024 * 1024, float('inf')) # 5MB以上
]
BIN_LABELS = ["0-500KB", "500KB-5MB", "5MB+"]
ALG_COLORS = ['#1f77b4']  # 不同算法的颜色
BAR_WIDTH = 0.2  # 柱状图宽度

def calculate_slowdown(real_fct, theoretical_fct):
    """计算FCT slowdown，确保值不小于1"""
    slowdown = real_fct / theoretical_fct
    return max(1.0, slowdown)

def process_file(file_path):
    """
    处理单个文件，返回三个区间(small, medium, large)的FCT slowdown平均值
    文件格式: 流id 流大小(Bytes) 理论FCT(ns) 真实FCT(ns)
    """
    slowdowns = [[] for _ in range(len(SIZE_BINS))]
    
    try:
        with open(file_path, 'r') as f:
            for line in f:
                parts = line.split()
                if len(parts) < 4:
                    continue
                
                flow_size = float(parts[1])
                theoretical_fct = float(parts[2])
                real_fct = float(parts[3])
                slowdown = calculate_slowdown(real_fct, theoretical_fct)
                
                for i, (lower, upper) in enumerate(SIZE_BINS):
                    if lower <= flow_size < upper:
                        slowdowns[i].append(slowdown)
                        break
    
    except Exception as e:
        print(f"Error processing file {file_path}: {str(e)}")
        return [np.nan, np.nan, np.nan]
    
    return [np.mean(bin_data) if bin_data else np.nan for bin_data in slowdowns]

def main():
    # 存储结果的数据结构: results[bin_index][cdf][algorithm]
    results = defaultdict(lambda: defaultdict(lambda: defaultdict(float)))
    
    # 处理所有文件
    for cdf in CDF_CHOICES:
        for alg in ALGORITHMS:
            # file_name = f"/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/large_trace/monitor_output/fct/{cdf}_load{SELECTED_LOAD}txt"  # 修改为实际文件名模式
            file_name = f"/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/large_trace/monitor_output/fct/websearch_load50.txt"  # 修改为实际文件名模式
            if not os.path.exists(file_name):
                print(f"Warning: File not found - {file_name}")
                continue
                
            bin_avgs = process_file(file_name)
            
            for bin_idx, avg in enumerate(bin_avgs):
                results[bin_idx][cdf][alg] = avg
    
    # 确定统一的纵坐标范围
    all_values = []
    for bin_idx in range(len(SIZE_BINS)):
        for cdf in CDF_CHOICES:
            for alg in ALGORITHMS:
                value = results[bin_idx][cdf][alg]
                if not np.isnan(value):
                    all_values.append(value)
    
    y_min = 0.9
    y_max = max(all_values) * 1.2 if all_values else 5.0  # 默认最大值

    # 输出格式选择
    output_format = 'png'

    # 为每个流大小区间创建单独的图表
    for bin_idx in range(len(SIZE_BINS)):
        fig, ax = plt.subplots(figsize=(12, 6))
        bin_label = BIN_LABELS[bin_idx]
        
        ax.set_title(f"FCT Slowdown (Load {SELECTED_LOAD}%): {bin_label}", fontsize=16)
        ax.set_xlabel("Workload Type", fontsize=14)
        ax.set_ylabel("Average FCT Slowdown", fontsize=14)
        ax.set_ylim(y_min, y_max)
        ax.grid(True, linestyle='--', alpha=0.7, axis='y')
        
        # 每组(CDF)的位置
        x = np.arange(len(CDF_CHOICES))  
        group_width = 0.8  # 每组的总宽度
        
        # 为每种算法绘制柱状图
        for alg_idx, alg in enumerate(ALGORITHMS):
            # 获取当前算法在所有CDF中的值
            values = [results[bin_idx][cdf][alg] for cdf in CDF_CHOICES]
            
            # 计算柱状图位置
            positions = x + alg_idx * BAR_WIDTH - (len(ALGORITHMS) * BAR_WIDTH - group_width)/2
            
            bars = ax.bar(
                positions, 
                values, 
                width=BAR_WIDTH, 
                color=ALG_COLORS[alg_idx],
                label=alg,
                edgecolor='black',
                linewidth=0.7
            )
            
            # 添加数值标签
            for bar in bars:
                height = bar.get_height()
                if not np.isnan(height):
                    ax.annotate(f'{height:.2f}',
                                xy=(bar.get_x() + bar.get_width()/2, height),
                                xytext=(0, 3),
                                textcoords="offset points",
                                ha='center', va='bottom',
                                fontsize=10)
        
        # 设置x轴
        ax.set_xticks(x)
        ax.set_xticklabels(CDF_CHOICES)
        ax.legend(loc='upper right', title="Algorithms")
        
        plt.tight_layout()
        output_file = f"/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/large_trace/_figure/fct_slowdown_load{SELECTED_LOAD}_{bin_label.replace('-', '_').lower()}.{output_format}"
        plt.savefig(output_file, bbox_inches='tight', dpi=300)
        print(f"图表已保存为 {output_file}")
        plt.close(fig)

if __name__ == "__main__":
    main()