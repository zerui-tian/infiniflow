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
# ALGORITHMS = ["dcqcn", "hpcc", "timely", "powerInt", "xpass", "valve"]  # 六种算法
# CDF_CHOICES = ["websearch", "hadoop", "datamining"]  # 三种场景
ALGORITHMS = ["dcqcn" ,"valve"]  
CDF_CHOICES = ["websearch","hadoop","datamining"]  

SELECTED_LOAD = 30  # 可以手动修改这个值(30, 50, 80)

SIZE_BINS = [
    (0, 500 * 1000),         # 0-500KB (不包含500KB)
    (500 * 1000, 5 * 1000 * 1000 + 1),  # 500KB-5MB (两边都包含5MB)
    (5 * 1000 * 1000 + 1, float('inf')) # 5MB以上
]
BIN_LABELS = ["0-500KB", "500KB-5MB", "5MB+"]
# ALG_COLORS = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b']  # 不同算法的颜色（新增valve颜色）
# BAR_WIDTH = 0.15  # 柱状图宽度（减小以适应更多算法）

ALG_COLORS = ['#1f77b4', '#ff7f0e']  # 不同算法的颜色（新增valve颜色）
BAR_WIDTH = 0.15  # 柱状图宽度（减小以适应更多算法）

# 全局映射表：flow_size -> theoretical_fct (使用dcqcn的值)
theoretical_fct_map = defaultdict(dict)  # 格式: {cdf: {flow_size: theoretical_fct}}

def calculate_slowdown(real_fct, theoretical_fct):
    """计算FCT slowdown，确保值不小于1"""
    slowdown = real_fct / theoretical_fct
    return max(1.0, slowdown)

def build_theoretical_fct_map(cdf):
    """
    为给定的cdf场景构建理论FCT映射表（使用dcqcn算法的数据）
    文件格式: 流id 流大小(Bytes) 理论FCT(ns) 真实FCT(ns)
    """
    file_name = f"/home/pnic/valve-opponent-ns3/simulator/ns-3.39/examples/PowerTCP/monitor_output/three_trace/fct/{cdf}_load{SELECTED_LOAD}/dcqcn.txt"
    if not os.path.exists(file_name):
        print(f"Warning: DCQCN file not found for {cdf} - {file_name}")
        return
    
    try:
        with open(file_name, 'r') as f:
            for line in f:
                parts = line.split()
                if len(parts) < 4:
                    continue
                
                flow_name = float(parts[0])
                theoretical_fct = float(parts[2])
                
                # 将流大小作为键，理论FCT作为值
                theoretical_fct_map[cdf][flow_name] = theoretical_fct
    
    except Exception as e:
        print(f"Error building theoretical FCT map for {cdf}: {str(e)}")

def process_file(file_path, cdf, algorithm):
    """
    处理单个文件，返回三个区间(small, medium, large)的FCT slowdown平均值
    文件格式: 流id 流大小(Bytes) 理论FCT(ns) 真实FCT(ns)
    """
    slowdowns = [[] for _ in range(len(SIZE_BINS))]
    flow_count = 0
    
    try:
        with open(file_path, 'r') as f:
            for line in f:
                parts = line.split()
                if len(parts) < 4:
                    continue
                flow_name = int(parts[0])
                flow_size = int(parts[1])
                real_fct = float(parts[3])
                
                # 获取理论FCT值
                if algorithm != "valve":
                    # dcqcn使用自己的理论FCT值
                    theoretical_fct = int(parts[2])
                else:
                    theoretical_fct = theoretical_fct_map[cdf][flow_name]
                
                slowdown = calculate_slowdown(real_fct, theoretical_fct)
                
                for i, (lower, upper) in enumerate(SIZE_BINS):
                    if lower <= flow_size < upper:
                        slowdowns[i].append(slowdown)
                        flow_count += 1
                        break
    
    except Exception as e:
        print(f"Error processing file {file_path}: {str(e)}")
        return [np.nan, np.nan, np.nan]
    
    if flow_count == 0:
        print(f"Warning: No valid flows found in {file_path}")
    
    return [np.mean(bin_data) if bin_data else np.nan for bin_data in slowdowns]

def main():
    # 首先为每个cdf场景构建理论FCT映射表（使用dcqcn的数据）
    for cdf in CDF_CHOICES:
        build_theoretical_fct_map(cdf)
    
    # 存储结果的数据结构: results[bin_index][cdf][algorithm]
    results = defaultdict(lambda: defaultdict(lambda: defaultdict(float)))
    
    # 处理所有文件
    for cdf in CDF_CHOICES:
        for alg in ALGORITHMS:
            file_name = f"/home/pnic/valve-opponent-ns3/simulator/ns-3.39/examples/PowerTCP/monitor_output/three_trace/fct/{cdf}_load{SELECTED_LOAD}/{alg}.txt"
            if not os.path.exists(file_name):
                print(f"Warning: File not found - {file_name}")
                continue
                
            bin_avgs = process_file(file_name, cdf, alg)
            
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
        fig, ax = plt.subplots(figsize=(12, 7))  # 增加高度以适应图例
        bin_label = BIN_LABELS[bin_idx]
        
        ax.set_title(f"FCT Slowdown (Load {SELECTED_LOAD}%): {bin_label}", fontsize=16)
        ax.set_xlabel("Workload Type", fontsize=14)
        ax.set_ylabel("Average FCT Slowdown", fontsize=14)
        ax.set_ylim(y_min, y_max)
        ax.grid(True, linestyle='--', alpha=0.7, axis='y')
        
        # 每组(CDF)的位置
        x = np.arange(len(CDF_CHOICES))  
        group_width = 0.95  # 每组的总宽度（增加以适应更多算法）
        
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
        output_file = f"/home/pnic/valve-opponent-ns3/simulator/ns-3.39/examples/PowerTCP/_figure/large-trace/fct_slowdown_load_test_{SELECTED_LOAD}_{bin_label.replace('-', '_').lower()}.{output_format}"
        plt.savefig(output_file, bbox_inches='tight', dpi=300)
        print(f"图表已保存为 {output_file}")
        plt.close(fig)

if __name__ == "__main__":
    main()