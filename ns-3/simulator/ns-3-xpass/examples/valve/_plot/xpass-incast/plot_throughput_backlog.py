import matplotlib.pyplot as plt
from matplotlib import rcParams

def plot_link_throughput(algorithms, throughput_file_paths, qlen_file_paths, colors, linestyles, source_node, destination_node):

    rcParams['font.size'] = 22  # 默认字体大小为 12，这里设置为 20
    rcParams['axes.labelsize'] = 22  # 坐标轴标签字体大小
    rcParams['xtick.labelsize'] = 22  # X 轴刻度字体大小
    rcParams['ytick.labelsize'] = 22 # Y 轴刻度字体大小
    rcParams['legend.fontsize'] = 22  # 图例字体大小

    
    plt.figure(figsize=(10, 6.5))
    throughput_waveform = plt.subplot(3,1,(2,3))
    qlen_waveform = plt.subplot(3,1,1)

    for alg in algorithms:
        # 初始化时间和吞吐率列表
        throughput_time_list = []
        throughput_list = []
        # 初始化时间和吞吐率列表
        qlen_time_list = []
        qlen_list = []

        # 打开文件并读取数据
        with open(throughput_file_paths[alg], 'r') as file:
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
                    throughput_time_list.append(time)
                    throughput_list.append(throughput)

        # 如果没有找到匹配的数据
        if not throughput_time_list:
            print(f"No throughput data of {alg} found for link {source_node} -> {destination_node}")
            return


        # 打开文件并读取数据
        with open(qlen_file_paths[alg], 'r') as file:
            for line in file:
                # 分割每行数据
                data = line.strip().split()
                if len(data) < 4:
                    continue
                
                # 提取源节点、目的节点、吞吐率和时间
                src = int(data[0])
                dst = int(data[1])
                qlen = float(data[2])/1e3 # 转换为 Mb
                time = float(data[3])/1e6

                # if time >= 7 :
                #     break
                # 如果匹配到指定的源节点和目的节点
                if src == source_node and dst == destination_node:
                    qlen_time_list.append(time)
                    qlen_list.append(qlen)
            

        # 如果没有找到匹配的数据
        if not qlen_time_list:
            print(f"No qlen data of {alg} found for link {source_node} -> {destination_node}")
            return

        # 绘制吞吐率随时间变化的曲线
        throughput_waveform.plot(throughput_time_list, throughput_list,linewidth=2,color=colors[alg],linestyle=linestyles[alg])
        qlen_waveform.plot(qlen_time_list, qlen_list,linewidth=2,color=colors[alg], label=alg,linestyle=linestyles[alg])

    # throughput_waveform.xlabel('Time (ms)')
    throughput_waveform.set_xlim(5, 90)
    # throughput_waveform.set_ylim(0, 40.5)
    throughput_waveform.set_ylabel('Throughput (Gbps)', labelpad=20)
    throughput_waveform.set_xlabel('Time (ms)')
    throughput_waveform.grid(True)
    
    qlen_waveform.set_ylabel('Backlog (Mb)')
    qlen_waveform.set_xlim(5, 90)
    qlen_waveform.set_ylim(0, 50)
    qlen_waveform.set_xticklabels([])
    qlen_waveform.axhline(y=25, color='red', linestyle='--', linewidth=2)
    qlen_waveform.text(45, 27, 'LCube peak backlog: 2MTU', color='red', fontsize=22, ha='center', va='bottom')
    qlen_waveform.grid(True)

    # plt.legend(loc='upper left', bbox_to_anchor=(1.02, 1), borderaxespad=0,labelspacing=1.2)
    plt.subplots_adjust(hspace=0.15)
    plt.savefig('/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_figure/fairness_evaluation/qlen_and_throughput_compare4.svg', format='svg',bbox_inches=None)


throughput_file_paths = {
    "xpass": '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/standing_queue/throughput.txt',

}

qlen_file_paths = {
    "xpass": '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/standing_queue/qlen.txt',
}

colors = {
    "xpass": "#457485",
}

linestyles = {
    "xpass": "-",
}

algorithms = ["xpass"]

source_node = 2
destination_node = 1

# 示例调用
throughput_file_path = '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/standing_queue/throughput.txt'  # 替换为你的txt文件路径
qlen_file_path = '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/standing_queue/qlen.txt'
plot_link_throughput(algorithms, throughput_file_paths, qlen_file_paths, colors, linestyles, 2, 1)