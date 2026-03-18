import os

def find_max_queue_length(filename, snode, dnode):
    """查找指定文件中特定节点对的最大队列长度"""
    max_queue_length = 0
    with open(filename, 'r') as file:
        for line in file:
            parts = line.strip().split()
            if len(parts) < 4:
                continue
            
            current_source = int(parts[0])
            current_dest = int(parts[1])
            queue_length = float(parts[2])

            if current_source == snode and current_dest == dnode:
                if queue_length > max_queue_length:
                    max_queue_length = queue_length
    return max_queue_length

def process_all_results(snode, dnode):
    # 定义路径常量
    base_dir = '../result'
    plot_dir = '../figure'
    output_file = os.path.join(plot_dir, 'qlen_nto1.txt')
    
    # 需要处理的文件夹和算法文件
    folders = ['4to1', '6to1', '8to1', '10to1']

    # 确保输出目录存在
    os.makedirs(plot_dir, exist_ok=True)

    # 处理所有数据
    with open(output_file, 'w') as fout:
        for folder in folders:
            folder_path = os.path.join(base_dir, folder)
            filename = os.path.join(folder_path, f"qlen.txt")
            
            # 如果文件不存在则跳过
            if not os.path.exists(filename):
                print(f"Warning: File {filename} not found, skipping...")
                continue
            
            # 获取最大队列长度
            max_len = find_max_queue_length(filename, snode, dnode)
            
            # 写入结果
            fout.write(f"{folder} {max_len:.2f}\n")

if __name__ == "__main__":
    # 用户指定的源节点和目的节点
    snode = 10  # 源节点
    dnode = 11  # 目的节点

    process_all_results(snode, dnode)
    print("Processing completed. Results saved to plot/qlen_nto1.txt")