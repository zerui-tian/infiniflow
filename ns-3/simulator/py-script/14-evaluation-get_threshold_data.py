import os
import re
import pandas as pd
import shutil
from collections import defaultdict

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

REPO_ROOT = os.path.realpath(os.path.join(SCRIPT_DIR, "../../.."))

output_dir = os.path.join(REPO_ROOT, "ns-3/simulator/result/data/14-evaluation")

metrics = ["threshold"]
for m in metrics:
    os.makedirs(os.path.join(output_dir, m), exist_ok=True)

# ==============================
# 正则匹配模式
# ==============================
pattern = re.compile(
    r"Time\s+(?P<time>\d+)"
    r"(?:\s+fccl\s+(?P<fccl>\d+))?"
    r"(?:\s+fctbs\s+(?P<fctbs>\d+))?"
    r"(?:\s+threshold\s+(?P<threshold>\d+))?"
    r"(?:\s+node\s+(?P<node>\d+)\s+port\s+(?P<port>\d+)\s+qIndex\s+(?P<qIndex>\d+))"
)

# ==============================
# 处理单个文件函数
# ==============================
def process_log_file(file_path):
    print(f"📂 正在处理: {os.path.basename(file_path)}")
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

    # 输出结果
    for m in metrics:
        metric_dir = os.path.join(output_dir, m)

        for key, records in data[m].items():
            df = pd.DataFrame(records, columns=["Time", m])
            df = df.sort_values("Time")
            output_path = os.path.join(metric_dir, f"{key}.csv")
            df.to_csv(output_path, index=False)
            print(f"   ✅ {m:<10} -> {key}.csv ({len(df)} 行)")

# ==============================
# 主逻辑：批量处理所有 .out 文件
# ==============================
filename = os.path.join(REPO_ROOT, "ns-3/simulator/result/data/14-evaluation/14-evaluation.out")
process_log_file(filename)

print("\n🎯 所有文件与 metric 分类输出完成！")
