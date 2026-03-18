import matplotlib.pyplot as plt
from matplotlib import rcParams
import sys

def plot_flow_rates(file_path, flow_ids):

    rcParams['font.size'] = 22  # 默认字体大小为 12，这里设置为 20
    rcParams['axes.labelsize'] = 22  # 坐标轴标签字体大小
    rcParams['xtick.labelsize'] = 22  # X 轴刻度字体大小
    rcParams['ytick.labelsize'] = 22 # Y 轴刻度字体大小
    rcParams['legend.fontsize'] = 22  # 图例字体大小
    # 初始化一个字典来存储每个 flow 的时间和速率数据
    flow_data = {flow_id: {'time': [], 'rate': []} for flow_id in flow_ids}

    # 打开文件并读取数据
    with open(file_path, 'r') as file:
        for line in file:
            # 分割每行数据
            data = line.strip().split()
            if len(data) < 3:
                continue
            
            # 提取 flow id、发送速率和时间
            flow_id = int(data[0])
            rate = float(data[1])
            time = float(data[2])/1e6

            # 如果 flow id 在指定的集合中，则记录数据
            if flow_id in flow_data:
                flow_data[flow_id]['time'].append(time)
                flow_data[flow_id]['rate'].append(rate)

    # 如果没有找到匹配的数据
    if all(len(flow_data[flow_id]['time']) == 0 for flow_id in flow_ids):
        print(f"No data found for the specified flow ids: {flow_ids}")
        return

    # 绘制每个 flow 的发送速率随时间变化的曲线
    plt.figure(figsize=(10, 4.5))
    colors = ["#b11810", "#f9c453", "#457485", "#173341"]
    for i, flow_id in enumerate(flow_ids):
        if flow_data[flow_id]['time']:  # 确保有数据
            plt.plot(flow_data[flow_id]['time'], flow_data[flow_id]['rate'], 
                     linestyle='-', label=f'Flow {i+1}', color=colors[i], linewidth=2.8)

    plt.xlabel('Time (ms)')
    plt.xlim(5, 85)
    plt.ylabel('Sending Rate (Gbps)')
    plt.grid(True)
    plt.legend()

    # 保存图像到指定路径
    plt.savefig('/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_figure/fairness_evaluation/fairness.'+sys.argv[1], format= sys.argv[1],bbox_inches='tight')

# 示例调用
if len(sys.argv) <= 1:
    print(sys.argv[0],": enter the output format!")
    exit(0)
else:
    file_path = '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/fairness_evaluation/monitor_output/sending_rate.txt'  # 替换为你的 txt 文件路径
    # flow_ids = {100, 101, 102, 103, 104}  # 替换为你要查询的 flow id 集合
    flow_ids = {100, 101, 102, 103} 

    plot_flow_rates(file_path, flow_ids)