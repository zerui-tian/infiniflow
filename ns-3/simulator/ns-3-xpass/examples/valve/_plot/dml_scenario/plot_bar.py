import matplotlib.pyplot as plt
import numpy as np

def plot_bars(means, models, algorithms, colors):
    plt.figure(figsize=(3.5, 1.2))  # 更小更宽的图像尺寸
    bar_width = 0.005
    gap = 0.001
    group_gap = 0.01
    x = np.array([0, 0 + len(algorithms) * (bar_width + gap) + group_gap])

    for i, alg in enumerate(algorithms):
        y_pos = [means[model][i] for model in models]
        plt.bar(
            x + i * (bar_width + gap),
            y_pos,
            width=bar_width,
            color=colors[alg],
            label=alg,
            edgecolor='black',
            linewidth=0.5,
        )

    plt.ylabel("Extra JCT (ms)", fontsize=7)
    plt.xticks(
        x + ((len(algorithms) * (bar_width + gap)) - gap) / 2 - 0.0025,
        [m.upper() for m in models],
        fontsize=7
    )
    plt.yticks(np.arange(0, 280, 50), fontsize=8)
    plt.ylim(0, 280)
    plt.grid(True, axis='y', linestyle='--', alpha=0.4, linewidth=0.5)

    legend = plt.legend(
        title=None,
        fontsize=7,
        loc='center left',
        bbox_to_anchor=(1.0, 0.5),
        frameon=True,
        fancybox=False,
    )
    legend.get_frame().set_linewidth(0.8)
    plt.tight_layout(pad=0.5)
    plt.savefig("/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_figure/dml_scenario/muti_means_bar_compact.pdf", dpi=300)
    plt.show()


if __name__ == "__main__":
    means = {
        "vgg16": [266, 73, 142, 126, 1477 - 1417, 29],
        "resnet50": [55, 18, 25, 20, 275 - 261, 12]
    }
    models = ["vgg16", "resnet50"]
    algorithms = ["TIMELY", "DCQCN", "HPCC", "PowerTCP", "Expresspass", "Valve"]
    colors = {
        "TIMELY": "#b11810",
        "DCQCN": "#f9c453",
        "HPCC": "#457485",
        "PowerTCP": "#173341",
        "Expresspass": "#6A0DAD",
        "Valve": "#FFFFFF",  # 如果你希望 Valve 柱子有边框可以加 edgecolor
    }
    plot_bars(means, models, algorithms, colors)
