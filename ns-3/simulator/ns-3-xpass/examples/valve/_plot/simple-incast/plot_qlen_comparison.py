import matplotlib.pyplot as plt
import os
import numpy as np
import sys

def plot_multiple_qlen_curves(size=4):
    alg_names = ["dcqcn", "powerInt", "hpcc", "timely", "expresspass"]
    alg_actual_name = {
        "dcqcn": "DCQCN",
        "powerInt": "PowerTCP",
        "hpcc": "HPCC",
        "timely": "TIMELY",
        "expresspass": "ExpressPass"
    }

    folder_path = "/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/simple-incast/qlen" + str(size) + "to1"
    fontsize = 12

    # 时间范围设置（仿真时间 ns）
    time_start_ns = 1_000_000
    time_end_ns = 1_500_000  # 将右边扩展至 500 μs

    if int(size) == 4:
        plt.figure(figsize=(3, 3))  # 图像长宽比为 1:1
    else:
        plt.figure(figsize=(2.75, 3))

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
                            time_us = (time_ns - time_start_ns) / 1_000
                            qlen_mb = qlen_kb / 1000
                            time_list.append(time_us)
                            qlen_list.append(qlen_mb)

            if time_list:
                plt.plot(time_list, qlen_list, label=alg_actual_name[alg])
            else:
                print(f"[Warning] No valid data for {alg}")

        except FileNotFoundError:
            print(f"[Error] File not found: {file_path}")

    # 添加 iRTT 参考线
    plt.axvline(x=30, color='red', linestyle='--', linewidth=2)
    plt.text(40, 18.5, 'RTT', color='red', fontsize=fontsize)  # 图内标注

    # 横坐标和纵坐标刻度设置
    plt.xticks(np.arange(0, 501, 200), fontsize=fontsize)
    plt.yticks(np.arange(0, 21, 4), fontsize=fontsize)

    plt.xlabel('Time (μs)', fontsize=fontsize)
    if int(size) == 4:
        plt.ylabel('Backlog (Mb)', fontsize=fontsize)
        plt.legend()
    plt.grid(True, linestyle='--', alpha=0.5)
    plt.tight_layout()

    output_path = "/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_figure/simple-incast/qlen_comparison_notype3_" + str(size) + "to1.pdf"
    plt.savefig(output_path, dpi=600)
    print(f"[Done] Plot saved to {output_path}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python plot_qlen_comparison.py <size>")
        sys.exit(1)
    plot_multiple_qlen_curves(sys.argv[1])
