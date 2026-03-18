import os
import re
import pandas as pd
import shutil
from collections import defaultdict

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

output_dir = os.path.join(REPO_ROOT, "ns-3/simulator/result/data/18-dt-limitation") 

os.makedirs(output_dir, exist_ok=True)

metrics = ["inflight"]
for m in metrics:
    os.makedirs(os.path.join(output_dir, m), exist_ok=True)


pattern = re.compile(
    r"Time\s+(?P<time>\d+)"
    r"(?:\s+fccl\s+(?P<fccl>\d+))?"
    r"(?:\s+fctbs\s+(?P<fctbs>\d+))?"
    r"(?:\s+inflight\s+(?P<inflight>\d+))?"
    r"(?:\s+abr\s+(?P<abr>\d+))?"
    r"(?:\s+remainBS\s+(?P<remainBS>\d+))?"
    r"(?:\s+threshold\s+(?P<threshold>\d+))?"
    r"(?:\s+ingress_bytes\s+(?P<ingress_bytes>\d+))?"
    r"(?:\s+egress_bytes\s+(?P<egress_bytes>\d+))?"
    r"(?:\s+node\s+(?P<node>\d+)\s+port\s+(?P<port>\d+)\s+qIndex\s+(?P<qIndex>\d+))"
)

def process_log_file(file_path):
    print(f"📂 processing: {os.path.basename(file_path)}")
    data = {m: defaultdict(list) for m in metrics}

    with open(file_path, "r") as f:
        for line in f:
            match = pattern.search(line)
            if not match:
                continue

            gd = match.groupdict()
            node, port, qIndex = gd["node"], gd["port"], gd["qIndex"]
            time = int(gd["time"])
            key = f"node{node}_port{port}_q{qIndex}"

            for m in metrics:
                val = gd.get(m)
                if val is not None:
                    data[m][key].append((time, int(val)))

    for m in metrics:
        metric_dir = os.path.join(output_dir, m)

        for key, records in data[m].items():
            df = pd.DataFrame(records, columns=["Time", m])
            df = df.sort_values("Time")
            output_path = os.path.join(metric_dir, f"{key}.csv")
            df.to_csv(output_path, index=False)
            print(f"   ✅ {m:<10} -> {key}.csv ({len(df)} 行)")

filename = os.path.join(REPO_ROOT, "ns-3/simulator/result/data/18-dt-limitation/18-dt-limitation.out")
process_log_file(filename)

print("\n🎯 finished！")
