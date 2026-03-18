import matplotlib.pyplot as plt
import sys

def plot_flow_rates(file_path, flow_ids):
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
    plt.figure(figsize=(10, 6))
    for flow_id in flow_ids:
        if flow_data[flow_id]['time']:  # 确保有数据
            plt.plot(flow_data[flow_id]['time'], flow_data[flow_id]['rate'], linestyle='-', label=f'Flow {flow_id}')

    plt.xlabel('Time (ms)')
    plt.ylabel('Sending Rate (Gbps)')
    plt.title('Sending Rate over Time for Flows')
    plt.grid(True)
    plt.legend()

    # 保存图像到指定路径
    plt.savefig('/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_figure/'+ sys.argv[1] +'/sending_rate.'+sys.argv[2], format= sys.argv[2],bbox_inches='tight')

# 示例调用
if len(sys.argv) <= 2:
    print(sys.argv[0],": enter your simulation name and the output format!")
    exit(0)
else:
    file_path = '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/'+ sys.argv[1] +'/monitor_output/sending_rate.txt'  # 替换为你的 txt 文件路径
    # flow_ids = {100, 101, 102, 103, 104}  # 替换为你要查询的 flow id 集合
    flow_ids = {100, 101, 102, 103} 

    plot_flow_rates(file_path, flow_ids)