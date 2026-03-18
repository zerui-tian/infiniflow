import matplotlib.pyplot as plt
from matplotlib import rcParams

def plot_link_throughput(algorithms, throughput_file_paths, qlen_file_paths, fig_path, colors, linestyles, source_node, destination_node):

    rcParams['font.size'] = 22  # 默认字体大小为 12，这里设置为 20
    rcParams['axes.labelsize'] = 22  # 坐标轴标签字体大小
    rcParams['xtick.labelsize'] = 22  # X 轴刻度字体大小
    rcParams['ytick.labelsize'] = 22 # Y 轴刻度字体大小
    rcParams['legend.fontsize'] = 22  # 图例字体大小
    rcParams['pdf.fonttype'] = 42
    rcParams['ps.fonttype'] = 42

    
    plt.figure(figsize=(8, 6.5))
    throughput_waveform = plt.subplot(3,1,(2,3))
    qlen_waveform = plt.subplot(3,1,1)

    for alg in algorithms:
        # 初始化时间和吞吐率列表
        throughput_time_list = []
        throughput_list = []
        # 初始化时间和吞吐率列表
        qlen_time_list = []
        qlen_list = []

        if alg == "Valve":
            source_node = 4
            destination_node = 5

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
                qlen = float(data[2])/1e3  # 转换为 Mb
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
    throughput_waveform.set_xlim(5, 100)
    # throughput_waveform.set_ylim(0, 40.5)
    throughput_waveform.set_ylabel('Throughput (Gbps)', labelpad=18)
    throughput_waveform.set_xlabel('Time (ms)')
    throughput_waveform.grid(True)
    
    qlen_waveform.set_ylabel('Backlog (Mb)', labelpad=20)
    qlen_waveform.set_xlim(5, 100)
    qlen_waveform.set_ylim(0, 10)
    qlen_waveform.set_xticklabels([])
    # qlen_waveform.axhline(y=25, color='red', linestyle='--', linewidth=2)
    if "Valve" in algorithms:
        plt.annotate(r'Valve peak backlog: 2MTU',
            xy=(42, 0), xycoords='data',
            xytext=(10, 30), textcoords='offset points', fontsize=20, color='red',
            arrowprops=dict(arrowstyle='->', connectionstyle='arc3, rad=.2'))
    # qlen_waveform.text(65, 3, 'Valve peak backlog: 2MTU', color='red', fontsize=22, ha='center', va='bottom')
    qlen_waveform.grid(True)

    plt.legend(loc='lower center', bbox_to_anchor=(0.5, 1.1), ncol=2, borderaxespad=0,labelspacing=1.2)
    plt.subplots_adjust(hspace=0.15)
    plt.savefig(fig_path, format='pdf',bbox_inches='tight')


qlen_file_paths = {
    "DCQCN": '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/fairness_evaluation/throughput_and_qlen/qlen-compare/dcqcn.txt',
    "PowerTCP": '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/fairness_evaluation/throughput_and_qlen/qlen-compare/powerInt.txt',
    "HPCC": '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/fairness_evaluation/throughput_and_qlen/qlen-compare/hpcc.txt',
    "TIMELY": '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/fairness_evaluation/throughput_and_qlen/qlen-compare/timely.txt',
    "Valve": '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/fairness_evaluation/throughput_and_qlen/qlen-compare/valve.txt',
    "ExpressPass": '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/fairness_evaluation/throughput_and_qlen/qlen-compare/xpass.txt',
    # "LCube": '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/result/throughput_and_qlen/throughput_lcube.txt'
}

throughput_file_paths = {
    "DCQCN": '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/fairness_evaluation/throughput_and_qlen/throughput-compare/dcqcn.txt',
    "PowerTCP": '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/fairness_evaluation/throughput_and_qlen/throughput-compare/powerInt.txt',
    "HPCC": '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/fairness_evaluation/throughput_and_qlen/throughput-compare/hpcc.txt',
    "TIMELY": '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/fairness_evaluation/throughput_and_qlen/throughput-compare/timely.txt',
    "Valve": '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/fairness_evaluation/throughput_and_qlen/throughput-compare/valve.txt',
    "ExpressPass": '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/fairness_evaluation/throughput_and_qlen/throughput-compare/xpass.txt',
    # "LCube": '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/result/throughput_and_qlen/qlen_lcube.txt'
}

colors = {
    "DCQCN": "#457485",
    "TIMELY": "#b11810",
    "PowerTCP": "#457485",
    "HPCC": "#b11810",
    "ExpressPass": "#457485",
    "Valve": "#b11810"
}

linestyles = {
    "DCQCN": "-",
    "PowerTCP": "-",
    "HPCC": "-",
    "TIMELY": "-",
    "ExpressPass": "-",
    "Valve": "-"
}

fig_paths = [
    '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_figure/fairness_evaluation/qlen_and_throughput_compare_dcqcn_timely.pdf',
    '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_figure/fairness_evaluation/qlen_and_throughput_compare_hpcc_powertcp.pdf',
    '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_figure/fairness_evaluation/qlen_and_throughput_compare_xpass_valve.pdf',
]

algorithms = ["TIMELY", "DCQCN"]
plot_link_throughput(algorithms, throughput_file_paths, qlen_file_paths, fig_paths[0], colors, linestyles, 2, 1)
algorithms = ["HPCC", "PowerTCP"]
plot_link_throughput(algorithms, throughput_file_paths, qlen_file_paths, fig_paths[1], colors, linestyles, 2, 1)
algorithms = ["ExpressPass", "Valve"]
plot_link_throughput(algorithms, throughput_file_paths, qlen_file_paths, fig_paths[2], colors, linestyles, 2, 1)