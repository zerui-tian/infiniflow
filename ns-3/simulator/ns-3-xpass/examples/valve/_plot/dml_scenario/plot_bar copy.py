import matplotlib.pyplot as plt
import numpy as np

def plot_bars(means, models, algorithms, colors):
    plt.figure(figsize=(10, 6))
    bar_width = 0.13
    gap = 0.02
    x = np.arange(len(models))

    for i, alg in enumerate(algorithms):
        y_pos = [means[model][i] for model in models]
        plt.bar(
            x + i * (bar_width + gap),
            y_pos,
            width=bar_width,
            color=colors[alg],
            label=alg,
            edgecolor='black',
            linewidth=1,
        )

    # plt.title("Extra JCT (ms) under Different Algorithms", fontsize=16)
    plt.xlabel("Model", fontsize=14)
    plt.ylabel("Extra JCT (ms)", fontsize=14)

    plt.xticks(
        x + (len(algorithms) * (bar_width + gap)) / 2 - (bar_width + gap) / 2,
        models,
        fontsize=14
    )

    plt.ylim(bottom=0)
    plt.grid(True, axis='y', linestyle='--', alpha=0.5)

    plt.legend(title="Algorithm", bbox_to_anchor=(1.05, 1), loc='upper left', fontsize=12)
    plt.tight_layout()
    plt.savefig("/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_figure/dml_scenario/muti_means_bar.png", dpi=300)
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
        "Valve": "#FFFFFF",
    }
    plot_bars(means, models, algorithms, colors)
