import matplotlib.pyplot as plt
import os
import numpy as np
import sys
def plot_multiple_qlen_curves(size = 4):
    alg_names = ["dcqcn", "powerInt", "hpcc", "timely", "expresspass"]
    alg_actual_name = {
        "dcqcn": "DCQCN",
        "powerInt" : "PowerTCP",
        "hpcc" : "HPCC",
        "timely" : "TIMELY",
        "expresspass" : "ExpressPass"
    }
    folder_path = "/home/pnic/valve-opponent-ns3/simulator/ns-3.39/examples/PowerTCP/monitor_output/simple-incast/qlen" + size + "to1" 

    fontsize = 12
    # 时间范围
    time_start_ns = 1_000_000
    time_end_ns = 1_400_000 # 200ms左右就全部结束了
    
    plt.figure(figsize=(6, 3))

    for alg in alg_names:
        file_path = os.path.join(folder_path, alg + ".txt")
        time_list = []
        qlen_list = []

        try:
            with open(file_path, 'r') as file:
                for line in file:
                    data = line.strip().split()
                    if len(data) < 4:
                        continue

                    src = int(data[0])
                    dst = int(data[1])
                    qlen_kb = float(data[2])
                    time_ns = float(data[3])

                    if src == 2 and dst == 1:
                        if time_start_ns <= time_ns <= time_end_ns:
                            # 将 1_000_000 ns 设为 0，单位转为 μs
                            time_us = (time_ns - time_start_ns) / 1_000
                            # 队列长度由 kb -> mb
                            qlen_mb = qlen_kb / 1000
                            time_list.append(time_us)
                            qlen_list.append(qlen_mb)

            if time_list:
                plt.plot(time_list, qlen_list, label=alg_actual_name[alg])
            else:
                print(f"[Warning] No valid data for {alg}")

        except FileNotFoundError:
            print(f"[Error] File not found: {file_path}")

    # 添加 iRTT 参考线（如需要可根据你的环境修改位置）

    plt.axvline(x=30, color='red', linestyle='--', linewidth=2)
    plt.text(52, 18.3, 'RTT', color='red', fontsize=fontsize)

    plt.xticks(np.arange(0, 400, 100), fontsize=fontsize)
    plt.yticks(np.arange(0, 20, 4), fontsize=fontsize)  # 从 0 到 12，步长 2
    plt.xlabel('Time (μs)', fontsize=fontsize)
    # plt.title(size + ' to 1 Queue Length Comparison (Src:2 -> Dst:1)')
    plt.grid(True, linestyle='--', alpha=0.5)
    if int(size) == 4:
        plt.ylabel('Backlog (Mb)', fontsize=fontsize)
        plt.legend()
    plt.tight_layout()

    output_path = "/home/pnic/valve-opponent-ns3/simulator/ns-3.39/examples/PowerTCP/_figure/simple-incast/qlen_comparison_" + size + "to1.png" # 中间有下划线的是最新的xpass
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    plt.savefig(output_path, dpi=600)
    print(f"[Done] Plot saved to {output_path}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python plot_qlen_comparison.py <size>")
        sys.exit(1)
    plot_multiple_qlen_curves(sys.argv[1])
