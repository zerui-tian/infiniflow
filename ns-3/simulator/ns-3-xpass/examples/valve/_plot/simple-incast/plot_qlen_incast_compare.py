import os
import matplotlib.pyplot as plt

# 定义算法对应的显示名和颜色、标记
algorithm_display = {
    'dcqcn': ('DCQCN', 'orange', 's'),
    'hpcc': ('HPCC', 'teal', 'D'),
    'powerInt': ('PowerTCP', 'black', '*'),
    'timely': ('TIMELY', 'red', 'o'),
    'expresspass': ('ExpressPass', 'purple', 'x'),
    'valve': ('Valve', 'blue', '^')   # 新增Valve
}

# 需要处理的 incast 场景及 x 轴对应值
folders = ['qlen4to1', 'qlen6to1', 'qlen8to1', 'qlen10to1']
incast_degrees = [4, 6, 8, 10]

# 指定源节点和目的节点（根据你的场景替换）
source_node = 2 
dest_node = 1

# 初始化存储算法结果的字典
results = {algo: [] for algo in algorithm_display}

# 从 monitor 文件夹下读取并处理每个算法在每个场景下的最大队列长度（过滤 src-dst）
for folder in folders:
    folder_path = os.path.join('/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/simple-incast/', folder)
    for algo in algorithm_display:
        if algo == 'valve':
            continue  # Valve不需要从文件中读取，直接赋值
        filename = os.path.join(folder_path, f'{algo}.txt')

        max_qlen = 0
        if os.path.exists(filename):
            with open(filename, 'r') as f:
                for line in f:
                    parts = line.strip().split()
                    if len(parts) < 4:
                        continue
                    try:
                        current_source = int(parts[0])
                        current_dest = int(parts[1])
                        qlen = float(parts[2]) / 1e3  # 转换为 Mb

                        if current_source == source_node and current_dest == dest_node:
                            if qlen > max_qlen:
                                max_qlen = qlen
                    except:
                        continue
        else:
            print(f"Warning: {filename} not found, skipping...")

        results[algo].append(max_qlen)

# Valve算法直接赋固定纵坐标
results['valve'] = [0.03, 0.03, 0.05, 0.07] #  # 根据实际情况设置Valve的队列长度

# 绘图
plt.figure(figsize=(6, 2))
for algo in algorithm_display:
    label, color, marker = algorithm_display[algo]
    plt.plot(incast_degrees, results[algo], label=label, color=color, marker=marker)
    # 将纵坐标数据标在每个点上方
    # for x, y in zip(incast_degrees, results[algo]):
    #     plt.text(x, y, f"{y:.2f}", fontsize=8, ha='center', va='bottom')

plt.xlabel("Incast Degree")
plt.ylabel("Peak Backlog (MB)")

plt.grid(True, linestyle='--', linewidth=0.5)

# 将图例放到右侧而不占用图内空间
plt.legend(loc='center left', bbox_to_anchor=(1, 0.5))

plt.tight_layout(rect=[0, 0, 0.85, 1])  # 为右侧图例腾出空间

# 输出保存
plt.savefig('/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_figure/simple-incast/qlen_incast_comparison.pdf', dpi=600)
plt.show()
