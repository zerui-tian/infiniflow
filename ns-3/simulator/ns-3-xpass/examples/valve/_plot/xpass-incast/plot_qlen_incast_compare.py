import os
import matplotlib.pyplot as plt

# 定义算法对应的显示名和颜色、标记
algorithm_display = {
    'dcqcn': ('DCQCN', 'orange', 's'),
    'hpcc': ('HPCC', 'teal', 'D'),
    'powerInt': ('PowerTCP', 'black', '*'),
    'timely': ('TIMELY', 'red', 'o'),
    'xpass': ('Xpass', 'purple', 'x')
}

# 需要处理的incast场景及x轴对应值
folders = ['qlen4to1', 'qlen6to1', 'qlen8to1', 'qlen10to1']
incast_degrees = [4, 6, 8, 10]

# 初始化存储算法结果的字典
results = {algo: [] for algo in algorithm_display}

# 从monitor文件夹下读取并处理每个算法在每个场景下的最大队列长度
for folder in folders:
    folder_path = os.path.join('/home/pnic/valve-opponent-ns3/simulator/ns-3.39/examples/PowerTCP/monitor_output/simple-incast/', folder)
    for algo in algorithm_display:
        filename = os.path.join(folder_path, f'{algo}.txt')
        
        max_qlen = 0
        if os.path.exists(filename):
            with open(filename, 'r') as f:
                for line in f:
                    parts = line.strip().split()
                    if len(parts) < 3:
                        continue
                    try:
                        qlen = float(parts[2]) / 1e6  # 转换为 Mb
                        if qlen > max_qlen:
                            max_qlen = qlen
                    except:
                        continue
        else:
            print(f"Warning: {filename} not found, skipping...")
        results[algo].append(max_qlen)

# 绘图
plt.figure(figsize=(6, 3))
for algo in algorithm_display:
    label, color, marker = algorithm_display[algo]
    plt.plot(incast_degrees, results[algo], label=label, color=color, marker=marker)
    for x, y in zip(incast_degrees, results[algo]):
        plt.text(x, y, f"{y:.2f}", fontsize=8, ha='center', va='bottom')

plt.xlabel("Incast Degree")
plt.ylabel("Peak Backlog (Mb)")
plt.grid(True, linestyle='--', linewidth=0.5)
plt.legend()
plt.tight_layout()

# 输出保存
os.makedirs('figure', exist_ok=True)
plt.savefig('figure/qlen_incast_comparison.png', dpi=300)
plt.show()
