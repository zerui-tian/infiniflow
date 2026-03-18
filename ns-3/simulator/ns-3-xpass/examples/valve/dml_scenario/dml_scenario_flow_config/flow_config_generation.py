def split_and_generate_files(input_dir, output_dir, n, m, d, p):
    input_file1_name = input_dir + '/train_1.txt'
    input_file2_name = input_dir + '/train_2.txt'

    compute_times = []
    # 读取原始文件并提取 compute_time 的值
    with open(input_file1_name, 'r') as input_file:
        lines = input_file.readlines()
    
    for line in lines:
        if 'compute_time' in line:
            parts = line.split()
            print(parts[5])
            t = float(parts[5])  # 提取 compute_time 的值
            if t < 1:
                compute_times.append(t)

    with open(input_file2_name, 'r') as input_file:
        lines = input_file.readlines()
    
    for line in lines:
        if 'compute_time' in line:
            parts = line.split()
            print(parts[5])
            t = float(parts[5])  # 提取 compute_time 的值
            if t < 1:
                compute_times.append(t)
    
    # 将 compute_times 分成若干组，每组 n 个条目
    for i in range(0, int(len(compute_times)/n)*n, n):
        group = compute_times[i:i+n]
        output_file_name = f"{output_dir}/iteration_{i//n + 1}.txt"
        
        with open(output_file_name, 'w') as output_file:
            j = 3
            output_file.write(f"{n}\n")  # 写入 n
            for t in group:
                output_file.write(f"{j} {d} {p} {100+j} {m} {t}\n")
                j = j + 1

# 示例调用
model = ["vgg16","resnet50"]
model_size = [138357544*4, 25502912*4]
n = 16  # 每个文件包含的条目数
d = 0  # 目的主机编号
p = 1  # 流量优先级

for i in range(2):
    input_dir = '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/dml_scenario/cnn_train_comp_time/'+model[i]
    output_dir = '/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/dml_scenario/dml_scenario_flow_config/' + model[i]
    split_and_generate_files(input_dir, output_dir, n, model_size[i], d, p)