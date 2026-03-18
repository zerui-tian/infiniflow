import numpy as np
import random
import os

# ----- CDF Definitions -----
cdf_sets = {
    "websearch": [
        (2000, 0.0),
        (2100, 0.02),
        (2500, 0.05),
        (6000, 0.1),
        (10000, 0.15),
        (20000, 0.2),
        (30000, 0.3),
        (50000, 0.4),
        (80000, 0.53),
        (200000, 0.6),
        (1000000, 0.7),
        (2000000, 0.8),
        (5000000, 0.9),
        (10000000, 0.97),
        (30000000, 1)
    ],
    "hadoop": [
        (325, 0),
        (1177, 0.01728594),
        (4264, 0.022426176),
        (15443, 0.027658305),
        (28000, 0.06),
        (37323, 0.104879491),
        (51067, 0.335908276),
        (74908, 0.515476971),
        (93293, 0.641894623),
        (141819, 0.772500294),
        (237853, 0.889446481),
        (573344, 0.957208265),
        (2076744, 0.965639401),
        (7521945, 0.9687178),
        (27244995, 0.974237092),
        (98686787, 0.983862831),
        (223092956, 1)
    ],
    "datamining": [
        (100, 0),
        (180, 0.085),
        (250, 0.14),
        (560, 0.33),
        (900, 0.47),
        (1100, 0.55),
        (1870, 0.65),
        (3160, 0.7),
        (10000, 0.8),
        (100001, 0.874),
        (400000, 0.9),
        (1850000, 0.95),
        (10000000, 0.97),
        (30000000, 0.98),
        (100000000, 0.99),
        (250000000, 0.995),
        (1000000000, 1)
    ]
}

# ----- Compute average flow sizes -----
def compute_avg_flow_size(cdf_points):
    avg_size = 0.0
    last_prob = 0.0
    for i in range(len(cdf_points)):
        size, prob = cdf_points[i]
        delta_prob = prob - last_prob
        avg_size += size * delta_prob
        last_prob = prob
    return avg_size

avg_sizes = {}
for name, cdf in cdf_sets.items():
    avg_size = compute_avg_flow_size(cdf)
    avg_sizes[name] = avg_size
    print(f"Average flow size for {name}: {avg_size:.2f} bytes")

# ----- Compute lambda from load -----
# load = lambda_rate * avg_flow_size / 400 * 50e9 => lambda_rate = load * 400 * 50e9 / avg_flow_size
def compute_lambda_for_load(load, avg_flow_size):
    return load * 400 * 50_000_000_000 / avg_flow_size

# ----- Flow Generation -----
def generate_flows(num_flows, lambda_rate, cdf_choice, output_file):
    cdf_points = cdf_sets[cdf_choice]
    sizes, probs = zip(*cdf_points)

    def sample_flow_size():
        u = random.random()
        for i in range(len(probs)):
            if u <= probs[i]:
                return sizes[i]
        return sizes[-1]

    inter_arrival_times = np.random.exponential(1 / lambda_rate, num_flows)
    start_times = np.cumsum(inter_arrival_times)

    os.makedirs(os.path.dirname(output_file), exist_ok=True)

    with open(output_file, 'w') as f:
        f.write(f"{num_flows}\n")
        for idx in range(num_flows):
            src = random.randint(0, 399)
            dst = random.randint(0, 399)
            while dst == src:
                dst = random.randint(0, 399)
            priority = 1
            size = sample_flow_size()
            dest_port = 10000 + idx
            start_time = start_times[idx]
            f.write(f"{src} {dst} {priority} {dest_port} {size} {start_time:.9f}\n")
    print(f"[✓] Flow trace generation completed: {output_file}")

# ----- Batch Generation for All -----
if __name__ == "__main__":
    num_flows = 10000
    loads = [0.3, 0.5, 0.8]
    cdf_choices = ["websearch", "hadoop", "datamining"]

    base_output_dir = "/home/pnic/valve-opponent-ns3/simulator/ns-3.39/examples/PowerTCP/flow"

    for cdf_choice in cdf_choices:
        avg_flow_size = avg_sizes[cdf_choice]
        for load in loads:
            lambda_rate = compute_lambda_for_load(load, avg_flow_size)
            output_file = f"{base_output_dir}/{cdf_choice}_load{int(load*100)}.txt"
            print(f"\n--- Generating for {cdf_choice} with load={load} ---")
            print(f"Computed lambda_rate: {lambda_rate:.2f} flows/sec")
            generate_flows(num_flows, lambda_rate, cdf_choice, output_file)
