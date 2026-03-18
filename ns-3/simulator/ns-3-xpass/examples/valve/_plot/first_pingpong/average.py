def calculate_average(file_path):
    values = []
    with open(file_path, 'r') as file:
        for line in file:
            parts = line.split()
            if len(parts) >= 3:  # 确保行有至少三个字段
                try:
                    value = float(parts[2])
                    if value != 0:  # 排除0值
                        values.append(value)
                except ValueError:
                    continue  # 忽略无效数字
    if not values:
        return 0.0  # 无非零值时返回0
    average = sum(values) / len(values)
    return round(average, 2)

# 示例使用
file5_avg = calculate_average('filtered_throughput5.txt')
file1_avg = calculate_average('filtered_throughput1.txt')
file05_avg = calculate_average('filtered_throughput05.txt')
print(f"filtered_throughput5.txt 平均值: {file5_avg}")
print(f"filtered_throughput1.txt 平均值: {file1_avg}")
print(f"filtered_throughput05.txt 平均值: {file05_avg}")