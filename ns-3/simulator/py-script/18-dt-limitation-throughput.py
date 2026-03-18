import matplotlib.pyplot as plt
import os

COLORS = [
    '#2274A5', 
    '#32936F', 
    '#FFBF00', 
    '#C00000',  
]


def plot_flow_rates(file_path, flow_ids, output_dir):
   
    plt.rcParams.update({
        "font.size": 17,
        "axes.titlesize": 17,
        "axes.labelsize": 17,
        "xtick.labelsize": 17,
        "ytick.labelsize": 17,
        "legend.fontsize": 17,
    })

    os.makedirs(output_dir, exist_ok=True)
    out_path = os.path.join(output_dir, '18-dt-limitation-throughput.png') 

    flow_ids = sorted(flow_ids)

    flow_data = {flow_id: {'time': [], 'rate': []} for flow_id in flow_ids}

    with open(file_path, 'r') as file:
        for line in file:
            data = line.strip().split()
            if len(data) < 3:
                continue

            flow_id = int(data[0])
            rate = float(data[1])
            time = float(data[2]) / 1e6  # ns -> ms

            if flow_id in flow_data:
                flow_data[flow_id]['time'].append(time)
                flow_data[flow_id]['rate'].append(rate)

    if all(len(flow_data[fid]['time']) == 0 for fid in flow_ids):
        print(f"No data found for the specified flow ids: {flow_ids}")
        return

    plt.figure(figsize=(4.5, 3))

    for idx, flow_id in enumerate(flow_ids):
        if flow_data[flow_id]['time']:
            plt.plot(
                flow_data[flow_id]['time'],
                flow_data[flow_id]['rate'],
                linestyle='-',
                linewidth=2,
                color=COLORS[idx],             
                label=f'Flow {flow_id % 5 + 1}'
            )

    plt.xlabel('Time (ms)')
    plt.ylabel('Throughput (Gbps)')
    plt.ylim(bottom=0)
    plt.xlim(0, 1.0)

    plt.grid(True, linestyle="--", alpha=0.6)
    plt.legend(
        frameon=False,
        ncol=2,
        columnspacing=0.8,
        handletextpad=0.4,
        labelspacing=0.3
    )

    # 保存图像
    plt.savefig(out_path, format='png', bbox_inches='tight', dpi=300)
    plt.close()

    print("Saved to:", out_path)


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

file_path = os.path.join(REPO_ROOT, "ns-3/simulator/result/data/18-dt-limitation/sendingrate.txt")
out_dir = os.path.join(REPO_ROOT, "ns-3/simulator/result/graph/")
flow_ids = {10000, 10001, 10002, 10003}

plot_flow_rates(file_path, flow_ids, out_dir)