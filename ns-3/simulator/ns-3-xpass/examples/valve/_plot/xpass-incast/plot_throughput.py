import sys
import matplotlib.pyplot as plt
from matplotlib import rcParams

def plot_link_throughput(file_path, source_node, destination_node):
    # 初始化时间和吞吐率列表
    time_list = []
    throughput_list = []

    # 打开文件并读取数据
    with open(file_path, 'r') as file:
        for line in file:
            # 分割每行数据
            data = line.strip().split()
            if len(data) < 4:
                continue
            
            # 提取源节点、目的节点、吞吐率和时间
            src = int(data[0])
            dst = int(data[1])
            throughput = float(data[2])
            time = float(data[3])/1e6

            # 如果匹配到指定的源节点和目的节点
            if src == source_node and dst == destination_node:
                time_list.append(time)
                throughput_list.append(throughput)

    # 如果没有找到匹配的数据
    if not time_list:
        print(sys.argv[0],f": No data found for link {source_node} -> {destination_node}")
        return

    # 绘制吞吐率随时间变化的曲线
    plt.plot(time_list, throughput_list)
    plt.xlabel('Time (ms)')
    plt.ylabel('Throughput (Gbps)')
    plt.grid(True)

    # 保存图像到指定路径
    plt.savefig('/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_figure/'+ sys.argv[1] +'/throughput.'+sys.argv[2], format= sys.argv[2],bbox_inches='tight')

# 示例调用
if len(sys.argv) <= 2:
    print(sys.argv[0],": enter your simulation name and the output format!")
    exit(0)
else:
    file_path = '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/'+ sys.argv[1] +'/monitor_output/throughput_4to1.txt'  # 替换为你的txt文件路径
    plot_link_throughput(file_path, 2, 1)