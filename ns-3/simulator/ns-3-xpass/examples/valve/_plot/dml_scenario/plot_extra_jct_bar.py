import json
import matplotlib.pyplot as plt
import numpy as np

def load_json(file_path):
    with open(file_path, 'r') as f:
        data = json.load(f)
    return data

def calculate_means(data, models, algorithms):
    means = {model: [] for model in models}
    for model in models:
        for alg in algorithms:
            vec = data[model][alg]["data"]  # 确保从正确的键获取数据
            mean = np.mean(vec)
            means[model].append(mean)
    return means

def plot_bars(means, models, algorithms, colors):
    # 设置图表大小
    plt.figure(figsize=(12, 6))
    
    # 柱形图参数
    bar_width = 0.15  # 柱子宽度
    gap = 0.05  # 柱形图之间的间隔
    
    # x轴位置计算
    x = np.arange(len(models))  # 模型数量
    
    # 绘制柱形图
    for i, alg in enumerate(algorithms):
        y_pos = [means[model][i] for model in models]
        plt.bar(
            x + i * (bar_width + gap), y_pos,
            width=bar_width,
            color=colors[alg],  # 使用指定颜色
            label=alg  # 每个算法使用不同颜色
        )
    
    # 图表标注
    plt.title("Time Range Statistics (Mean)", fontsize=20)
    plt.xlabel("Model", fontsize=20)
    plt.ylabel("Time Range", fontsize=20)
    
    # 设置 x 轴标签
    plt.xticks(
        x + (len(algorithms) * (bar_width + gap)) / 2,
        models,
        fontsize=20  # 调整字体大小
    )
    
    # 设置 Y 轴起点
    plt.ylim(bottom=0)  # Y 轴从 0 开始
    
    plt.grid(True, axis='y', linestyle='--', alpha=0.7)
    
    # 添加图例（算法）
    plt.legend(title="Algorithm", bbox_to_anchor=(1.05, 1), loc='upper left', fontsize=20)  # 图例放在右侧
    
    # 手动调整边距
    plt.subplots_adjust(left=0.1, right=0.8, bottom=0.2, top=0.9)  # 增加右侧边距
    
    # 保存和显示
    save_path = "multi_model_bar_means.png"
    plt.savefig(save_path, bbox_inches='tight')  # 确保保存时包含所有内容
    plt.show()

if __name__ == "__main__":
    file_path = "/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/dml_scenario/monitor_output/result_vectors.json"
    data = load_json(file_path)
    models = ["vgg16", "resnet50"]
    # algorithms = ["TIMELY", "DCQCN", "HPCC", "PowerTCP", "LCube"]  
    algorithms = ["Expresspass"]  
    colors = {
        "Expresspass": "#4C2A6E"
    }
    means = calculate_means(data, models, algorithms)
    plot_bars(means, models, algorithms, colors)