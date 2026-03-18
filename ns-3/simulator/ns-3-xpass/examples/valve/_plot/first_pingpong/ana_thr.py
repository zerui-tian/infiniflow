def filter_data_files(input_file, output_file, filter_prefix='0 2'):
    """
    过滤输入文件，将以指定前缀开头的行保存到输出文件
    
    参数:
        input_file: 输入文件路径
        output_file: 输出文件路径
        filter_prefix: 要保留行的前缀，默认为'1 2'
    """
    # 读取文件并过滤行
    with open(input_file, 'r') as f_in:
        # 只保留以指定前缀开头的行
        filtered_lines = [line for line in f_in if line.startswith(filter_prefix)]
    
    # 将结果写入新文件
    with open(output_file, 'w') as f_out:
        f_out.writelines(filtered_lines)
    
    # 返回统计信息
    return {
        'input_file': input_file,
        'output_file': output_file,
        'original_lines': sum(1 for _ in open(input_file)),
        'filtered_lines': len(filtered_lines)
    }

# 批量处理示例
if __name__ == "__main__":
    # 定义要处理的文件列表 (文件名, 过滤前缀)
    files_to_process = [
        ('throughput05.txt', 'filtered_data_throughput05.txt', '0 2'),
        ('throughput1.txt', 'filtered_data_throughput1.txt', '0 2'),
        ('throughput1d5.txt', 'filtered_data_throughput1d5.txt', '0 2'),
        ('throughput2.txt', 'filtered_data_throughput2.txt', '0 2'),
        ('throughput2d5.txt', 'filtered_data_throughput2d5.txt', '0 2'),
        ('throughput5.txt', 'filtered_data_throughput5.txt', '0 2')
    ]
    
    # 处理所有文件
    results = []
    for input_file, output_file, prefix in files_to_process:
        result = filter_data_files(input_file, output_file, prefix)
        results.append(result)
    
    # 打印处理结果
    print("\n处理结果:")
    print("=" * 50)
    for res in results:
        print(f"输入文件: {res['input_file']}")
        print(f"输出文件: {res['output_file']}")
        print(f"原始行数: {res['original_lines']}")
        print(f"过滤后行数: {res['filtered_lines']}")
        print("-" * 50)