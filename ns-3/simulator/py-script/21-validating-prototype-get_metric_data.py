import os
import re
import pandas as pd
from collections import defaultdict

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

output_dir = os.path.join(REPO_ROOT, "ns-3/simulator/result/data/21-validating-prototype") 
os.makedirs(output_dir, exist_ok=True)

PKT_SIZE = 1024 

metrics = ["inflight", "threshold", "remainBS", "credit"]
for m in metrics:
    os.makedirs(os.path.join(output_dir, m), exist_ok=True)

pattern = re.compile(
    r"Time\s+(?P<time>\d+)"
    r"(?:\s+fccl\s+(?P<fccl>\d+))?"
    r"(?:\s+fctbs\s+(?P<fctbs>\d+))?"
    r"(?:.*?inflight\s+(?P<inflight>\d+))?"
    r"(?:.*?remainBS\s+(?P<remainBS>\d+))?"
    r"(?:.*?threshold\s+(?P<threshold>\d+))?"
    r".*?\s+node\s+(?P<node>\d+)\s+port\s+(?P<port>\d+)\s+qIndex\s+(?P<qIndex>\d+)"
)

def process_log_file(file_path):
    print(f"📂 正在处理: {os.path.basename(file_path)}")

    data = {m: defaultdict(list) for m in metrics}

    with open(file_path, "r") as f:
        for line in f:
            match = pattern.search(line)
            if not match:
                continue

            gd = match.groupdict()

            time = int(gd["time"])
            node = gd["node"]
            port = gd["port"]
            qIndex = gd["qIndex"]
            key = f"node{node}_port{port}_q{qIndex}"

            for m in ["inflight", "threshold", "remainBS"]:
                val = gd.get(m)
                if val is not None:
                    packets = int(val) // PKT_SIZE
                    data[m][key].append((time, packets))

            fccl = gd.get("fccl")
            fctbs = gd.get("fctbs")
            if fccl is not None and fctbs is not None:
                credit_packets = (int(fccl) - int(fctbs)) // PKT_SIZE
                data["credit"][key].append((time, credit_packets))

    for m in metrics:
        metric_dir = os.path.join(output_dir, m)
        os.makedirs(metric_dir, exist_ok=True)

        for key, records in data[m].items():
            if not records:
                continue

            df = pd.DataFrame(records, columns=["Time", m])
            df = df.sort_values("Time")

            output_path = os.path.join(metric_dir, f"{key}.csv")
            df.to_csv(output_path, index=False)

            print(f"   ✅ {m:<10} -> {key}.csv ({len(df)} 行)")

filename = os.path.join(REPO_ROOT, "ns-3/simulator/result/data/21-validating-prototype/21-validating-prototype.out")

process_log_file(filename)

print("\n🎯 Finished")
