import os
import json

def process_file(file_path):
    min_start = float('inf')
    max_end = -float('inf')
    
    with open(file_path, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) < 2: # 看实际是几列，现在只输出2列，第一列是开始时间，第二列FCT时间
                continue  # 跳过格式错误的行
            start = int(parts[0]) #第一列，原本是第六列
            duration = int(parts[1]) #第二列 原本是第七列
            end = start + duration
            if start < min_start:
                min_start = start
            if end > max_end:
                max_end = end
    return max_end - min_start if max_end != -float('inf') else 0

def main():
    models = ["vgg16", "resnet50"]
    output_file = "/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/dml_scenario/monitor_output/result_vectors.json"

    result = {}

    for model in models:
        result[model] = []  # 保持原先只存 vector 的结构不变
        base_dir = os.path.join("/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/dml_scenario/monitor_output/", model)
        vector = []
        for i in range(1, 401):
            file_path = os.path.join(base_dir, f"fct_{i}.txt")
            print(file_path)
            if not os.path.exists(file_path):
                print(f"Warning: {file_path} not found.")
                vector.append(0)
                continue
            time_range = process_file(file_path)
            vector.append(time_range)
        
        # 存储到 result，保持 JSON 内容与之前一致
        result[model] = vector

        # 计算并打印平均值（以 ms 为单位）
        valid_values = [v for v in vector if v > 0]
        if valid_values:
            avg_ms = sum(valid_values) / len(valid_values) / 1e6
        else:
            avg_ms = 0
        print(f"{model} 平均JCT: {avg_ms:.2f} ms")

    # 将结果写入 JSON（内容结构与之前完全一致）
    with open(output_file, 'w') as f:
        json.dump(result, f, indent=4)

    print("Finish")

if __name__ == "__main__":
    # 先运行get_each_iteration_JCT.py脚本，生成result_vectors.json文件,再使用plot_bar.py脚本绘图
    main()
