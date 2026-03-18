import os
import re
import pandas as pd
from collections import defaultdict

# ==============================
# 配置
# ==============================
log_path = "dump_test/evaluation-NoAlgorithm.out"       # 原始输入文件路径
output_dir = "metrics_output" # 输出主目录
os.makedirs(output_dir, exist_ok=True)

# metric 列表
metrics = ["fccl", "fctbs", "inflight", "abr", "remainBS", "threshold"]
for m in metrics:
    os.makedirs(os.path.join(output_dir, m), exist_ok=True)

# ==============================
# 正则匹配模式
# ==============================
pattern = re.compile(
    r"Time\s+(?P<time>\d+)"
    r"(?:\s+fccl\s+(?P<fccl>\d+))?"
    r"(?:\s+fctbs\s+(?P<fctbs>\d+))?"
    r"(?:\s+inflight\s+(?P<inflight>\d+))?"
    r"(?:\s+abr\s+(?P<abr>\d+))?"
    r"(?:\s+remainBS\s+(?P<remainBS>\d+))?"
    r"(?:\s+threshold\s+(?P<threshold>\d+))?"
    r"(?:\s+node\s+(?P<node>\d+)\s+port\s+(?P<port>\d+)\s+qIndex\s+(?P<qIndex>\d+))"
)

# ==============================
# 数据收集结构
# ==============================
data = {m: defaultdict(list) for m in metrics}

# ==============================
# 读取与分类
# ==============================
with open(log_path, "r") as f:
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

# ==============================
# 输出 CSV 文件
# ==============================
for m in metrics:
    metric_dir = os.path.join(output_dir, m)
    for key, records in data[m].items():
        df = pd.DataFrame(records, columns=["Time", m])
        df = df.sort_values("Time")
        output_path = os.path.join(metric_dir, f"{key}.csv")
        df.to_csv(output_path, index=False)
        print(f"✅ Wrote {len(df)} rows -> {output_path}")

print("🎯 所有 metric 文件输出完成！")
