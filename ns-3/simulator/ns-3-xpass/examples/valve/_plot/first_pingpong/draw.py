import matplotlib.pyplot as plt
import numpy as np
import os

# 读取数据文件的函数
def read_data(filename):
    """
    读取过滤后的数据文件，提取时间和吞吐量
    
    参数:
        filename: 数据文件路径
        
    返回:
        times: 时间数组（毫秒）
        throughputs: 吞吐量数组（Gbps）
    """
    times = []
    throughputs = []
    with open(filename, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 4:
                try:
                    # 第四列时间（纳秒）转换为毫秒
                    time_ns = float(parts[3])
                    time_ms = time_ns / 1_000_000  # 纳秒转毫秒
                    throughput = float(parts[2])    # 第三列吞吐量(Gbps)
                    
                    # 排除0值
                    if throughput > 0:
                        times.append(time_ms)
                        throughputs.append(throughput)
                except ValueError:
                    # 忽略无法转换的数字
                    continue
    return np.array(times), np.array(throughputs)

# 定义要处理的文件列表及其标签
data_files = [
    {'file': 'filtered_data_throughput05.txt', 'label': '0.5Gbps token refresh rate'},
    {'file': 'filtered_data_throughput1.txt', 'label': '1Gbps token refresh rate'},
    {'file': 'filtered_data_throughput1d5.txt', 'label': '1.5Gbps token refresh rate'},
    {'file': 'filtered_data_throughput2.txt', 'label': '2Gbps token refresh rate'},
    {'file': 'filtered_data_throughput2d5.txt', 'label': '2.5Gbps token refresh rate'},
    {'file': 'filtered_data_throughput5.txt', 'label': '5Gbps token refresh rate'}
]

# 创建图表
plt.figure(figsize=(14, 8))

# 扩展样式列表以支持6个数据集
colors = ['b', 'r', 'g', 'm', 'c', 'orange']  # 添加橙色作为第六种颜色
line_styles = ['-', '--', '-.', ':', '-', '--']  # 添加虚线作为第六种线型
markers = ['o', 's', '^', 'D', 'v', 'x']  # 添加'x'作为第六种标记

# 读取并绘制所有数据文件
all_data = []
for i, data_info in enumerate(data_files):
    if os.path.exists(data_info['file']):
        times, throughputs = read_data(data_info['file'])
        
        # 计算平均值（排除0值）
        avg_throughput = np.mean(throughputs[throughputs > 0]) if any(throughputs > 0) else 0
        
        # 添加到图表
        plt.plot(times, throughputs, 
                 color=colors[i % len(colors)],
                 linestyle=line_styles[i % len(line_styles)],
                 marker=markers[i % len(markers)],
                 markersize=4,
                 markevery=20,  # 每隔20个点标记一次，避免过于密集
                 linewidth=1.5, 
                 label=f"{data_info['label']} (Avg: {avg_throughput:.2f} Gbps)")
        
        all_data.append((times, throughputs))
    else:
        print(f"警告: 文件 {data_info['file']} 不存在，跳过")

# 添加标题和标签
plt.title('Network Throughput Comparison with Different Token Refresh Rates', fontsize=16)
plt.xlabel('Time (milliseconds)', fontsize=14)
plt.ylabel('Throughput (Gbps)', fontsize=14)
plt.grid(True, linestyle='--', alpha=0.7)

# 将图例放在图表外部（顶部）以避免遮挡数据
plt.legend(fontsize=10, loc='upper center', bbox_to_anchor=(0.5, -0.15), 
           fancybox=True, shadow=True, ncol=2)

# 设置坐标轴范围
if all_data:
    # 找到所有时间序列的最小最大值
    min_time = min(min(times) for times, _ in all_data)
    max_time = max(max(times) for times, _ in all_data)
    max_throughput = max(max(throughputs) for _, throughputs in all_data)
    
    plt.xlim(min_time, max_time)
    plt.ylim(0, max_throughput * 1.15)

# 添加网格和次要网格
plt.grid(True, which='major', linestyle='-', linewidth='0.5', color='gray')
plt.grid(True, which='minor', linestyle=':', linewidth='0.5', color='lightgray')
plt.minorticks_on()

# 添加标注
plt.annotate('Periodic Fluctuations', 
             xy=(0.75, 0.85), 
             xycoords='axes fraction',
             fontsize=12, 
             ha='center',
             bbox=dict(boxstyle="round,pad=0.3", fc="yellow", ec="gray", alpha=0.5))

# 调整布局以容纳外部图例
plt.tight_layout()
plt.subplots_adjust(bottom=0.2)  # 为底部图例留出空间

# 保存高质量图片
plt.savefig('throughput_comparison_all.png', dpi=300, bbox_inches='tight')
print("图表已保存为 'throughput_comparison_all.png'")

# 显示图表
plt.show()

# 可选：创建汇总统计图表
if all_data:
    plt.figure(figsize=(10, 6))
    
    # 计算每个数据集的平均值
    averages = [np.mean(throughputs[throughputs > 0]) for _, throughputs in all_data]
    labels = [data_info['label'] for data_info in data_files if os.path.exists(data_info['file'])]
    
    # 创建柱状图
    bars = plt.bar(labels, averages, color=colors[:len(labels)])
    
    # 在柱子上方显示数值
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                 f'{height:.2f}', 
                 ha='center', va='bottom')
    
    plt.title('Average Throughput Comparison', fontsize=16)
    plt.xlabel('Token Refresh Rate', fontsize=14)
    plt.ylabel('Average Throughput (Gbps)', fontsize=14)
    plt.xticks(rotation=15)
    plt.grid(True, axis='y', linestyle='--', alpha=0.7)
    
    plt.tight_layout()
    plt.savefig('throughput_averages.png', dpi=300)
    print("汇总图表已保存为 'throughput_averages.png'")
    plt.show()