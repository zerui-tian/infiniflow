import os
import numpy as np
from collections import defaultdict

# 定义常量
ALGORITHMS = ["Valve"]
CDF_CHOICES = ["websearch", "datamining", "hadoop"]
LOAD_LEVELS = [30, 50, 80]

SIZE_BINS = [
    (0, 500 * 1024),                   # 0-500KB
    (500 * 1024, 5 * 1024 * 1024),     # 500KB-5MB
    (5 * 1024 * 1024, float('inf'))    # 5MB+
]
BIN_LABELS = ["0-500KB", "500KB-5MB", "5MB+"]

def calculate_slowdown(real_fct, theoretical_fct):
    """计算FCT slowdown，确保值不小于1"""
    slowdown = real_fct / theoretical_fct
    return max(1.0, slowdown)

def build_theoretical_fct_map(cdf, load):
    """
    为给定的 CDF 和 Load 构建理论 FCT 映射（使用 dcqcn 算法的数据）
    映射：flow_id → 理论FCT（ns）
    """
    file_path = f"/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/large_scale/fct_start_num_16/{cdf}_load{load}/dcqcn.txt"
    flow_theoretical_map = {}

    if not os.path.exists(file_path):
        print(f"Warning: DCQCN file not found for {cdf} load {load} - {file_path}")
        return flow_theoretical_map

    try:
        with open(file_path, 'r') as f:
            for line in f:
                parts = line.strip().split()
                if len(parts) < 4:
                    continue
                flow_id = parts[0]
                theoretical_fct = float(parts[2])
                flow_theoretical_map[flow_id] = theoretical_fct
    except Exception as e:
        print(f"Error reading theoretical FCT map: {e}")

    return flow_theoretical_map

def process_valve_file(file_path, theoretical_map):
    """
    使用 valve 的实际 FCT + dcqcn 的理论 FCT，计算 slowdown 并按流大小划分
    文件格式假设：... flow_id flow_size ... real_fct theoretical_fct ...
    实际字段顺序取决于格式，这里按 flow_id=0, flow_size=4, real=6
    """
    slowdowns = [[] for _ in range(len(SIZE_BINS))]

    try:
        with open(file_path, 'r') as f:
            for line in f:
                parts = line.strip().split()
                if len(parts) < 7:
                    continue

                flow_id = parts[3]
                flow_size = float(parts[4])
                real_fct = float(parts[6])

                if flow_id not in theoretical_map:
                    continue  # 无理论 FCT 匹配

                theoretical_fct = theoretical_map[flow_id]
                slowdown = calculate_slowdown(real_fct, theoretical_fct)

                for i, (lower, upper) in enumerate(SIZE_BINS):
                    if lower <= flow_size < upper:
                        slowdowns[i].append(slowdown)
                        break

    except Exception as e:
        print(f"Error processing Valve file {file_path}: {str(e)}")

    return [np.mean(bin_data) if bin_data else np.nan for bin_data in slowdowns]

def main():
    for cdf in CDF_CHOICES:
        for load in LOAD_LEVELS:
            # 1. 构建 dcqcn 理论 FCT 映射
            theoretical_map = build_theoretical_fct_map(cdf, load)

            # 2. 处理 valve 数据文件
            valve_file_path = f"/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_archive/large_scale/fct_start_num_16/{cdf}_load{load}/valve.txt"
            if not os.path.exists(valve_file_path):
                print(f"Warning: Valve file not found - {valve_file_path}")
                continue

            avg_slowdowns = process_valve_file(valve_file_path, theoretical_map)

            # 3. 打印结果
            for i, avg in enumerate(avg_slowdowns):
                bin_label = BIN_LABELS[i]
                print(f"[{cdf}][Load {load}%][{bin_label}]: Avg FCT Slowdown = {avg:.2f}")

if __name__ == "__main__":
    main()
