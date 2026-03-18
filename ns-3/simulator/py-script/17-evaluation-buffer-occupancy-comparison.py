#!/usr/bin/env python3
import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker

plt.rcParams.update({
    'font.size': 17,
    'axes.titlesize': 17,
    'axes.labelsize': 17,
    'xtick.labelsize': 17,
    'ytick.labelsize': 17,
    'legend.fontsize': 17,
    'figure.titlesize': 17
})

WORKLOAD_FILES = ["W3_load80.txt", "W4_load80.txt"]

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

BASE_DIR = os.path.join(REPO_ROOT, "ns-3/simulator/result/data/16-large-scale")

ALGO_NAMES = ["InfiniFlow", "CBFC alone", "PFC alone", "DCQCN + PFC", "HPCC + PFC", "BFC"]

algo_dirs = {
    "InfiniFlow": "infiniflow",
    "CBFC alone": "infiniband",
    "PFC alone": "pfc", 
    "DCQCN + PFC": "dcqcn",
    "HPCC + PFC": "hpcc",
    "BFC": "bfc"
}

STYLES = {
    "InfiniFlow": {'color': '#C00000', 'ls': '-',  'lw': 2, 'zorder': 10}, 
    "PFC alone":   {'color': '#1f77b4', 'ls': '-.', 'lw': 2},
    "DCQCN + PFC": {'color': '#9467bd', 'ls': '--', 'lw': 2},
    "CBFC alone":  {'color': '#2ca02c', 'ls': '-.', 'lw': 2},
    "HPCC + PFC":  {'color': '#E83F6F', 'ls': (0, (6, 3)), 'lw': 2},
    "BFC":         {'color': '#f47F38', 'ls': (0, (3, 1, 1, 1)), 'lw': 2},
}

OUT_DIR = os.path.join(REPO_ROOT, "ns-3/simulator/result/graph")

def load_qlen_data(file_path):
    if not os.path.isfile(file_path): 
        return np.array([])
    try:
        data = np.loadtxt(file_path, usecols=1)
        return data
    except: 
        print(f"  [WARNING] Failed to read file: {file_path}")
        return np.array([])

def plot_single_qlen_cdf(workload, algo_data_dict):
    plt.figure(figsize=(4.5, 3))
    ax = plt.gca()
    
    y_ticks = np.arange(0.8, 1.01, 0.05)
    
    for algo in ALGO_NAMES:
        data = algo_data_dict.get(algo, np.array([]))
        if data.size == 0: 
            continue
        
        sorted_data = np.sort(data)
        yvals = np.arange(len(sorted_data)) / float(len(sorted_data) - 1)
        
        style = STYLES.get(algo, {'color': 'black'})
        plt.plot(sorted_data, yvals, **style)

    ax.set_xscale('log')
    major_ticks = [1, 10, 100, 1000, 10000]
    ax.set_xticks(major_ticks)
    ax.xaxis.set_major_formatter(mticker.LogFormatterMathtext(base=10))

    ax.grid(True, which='major', axis='x', linestyle='--', linewidth=0.7, alpha=0.7)
    ax.grid(True, which='major', axis='y', linestyle='--', linewidth=0.7, alpha=0.7)
    
    ax.set_xlim(1, 10000)
    ax.set_ylim(0.8, 1.0)
    ax.set_yticks(y_ticks)
    ax.set_ylabel('CDF')
    ax.yaxis.set_major_formatter(mticker.FormatStrFormatter('%.2f'))
    ax.set_xlabel('Buffer Occupancy (KB)', fontsize=17)

    os.makedirs(OUT_DIR, exist_ok=True)
    filename = f"17-evaluation_buffer-occupancy_{workload.replace('.txt', '')}.png"
    save_path = os.path.join(OUT_DIR, filename)
    
    plt.tight_layout()
    plt.savefig(save_path, bbox_inches='tight', dpi=300)
    print(f"  [SUCCESS] Individual chart saved: {save_path}")
    plt.close()

def main():
    for workload in WORKLOAD_FILES:
        print(f"[PROCESS] Reading and plotting: {workload}")
        algo_data_dict = {}
        
        for algo_name in ALGO_NAMES:
            if algo_name in algo_dirs:
                algo_dir = algo_dirs[algo_name]
                file_path = os.path.join(BASE_DIR, algo_dir, "qlen", workload)
                
                data = load_qlen_data(file_path)
                if data.size > 0:
                    algo_data_dict[algo_name] = data
                else:
                    print(f"  [WARNING] File missing or empty: {file_path}")
        
        if algo_data_dict:
            plot_single_qlen_cdf(workload, algo_data_dict)
        else:
            print(f"  [ERROR] No data loaded for {workload}")

if __name__ == "__main__":
    main()