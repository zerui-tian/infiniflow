import re
import pandas as pd
import matplotlib.pyplot as plt
import os

# ------------------------
# 1. 日志文件路径（在这里改）
# ------------------------
log_path = "dump_test/evaluation-NoAlgorithm.out"

# ------------------------
# 2. 图像保存目录
# ------------------------
script_dir = os.path.dirname(os.path.abspath(__file__))
plot_dir = os.path.join(script_dir, "plots")
os.makedirs(plot_dir, exist_ok=True)

# ------------------------
# 3. 可调参数
# ------------------------
x_min = None          # ❗ None 表示自动按数据范围
x_max = None
# y_max = 40000         # ✅ 纵坐标最大值（超过的不画）

# ------------------------
# 4. 日志解析函数（支持 node + port + qIndex）
# ------------------------
def parse_simulation_log(log_path):
    data = []
    with open(log_path, 'r', encoding='utf-8') as f:
        for line in f:
            if "Time" not in line:
                continue

            # 提取时间
            time_match = re.search(r"Time\s+(\d+)", line)
            if not time_match:
                continue
            entry = {"Time": int(time_match.group(1))}

            # 提取各字段（独立搜索）
            for key in ["fccl", "fctbs", "inflight", "abr", "remainBS", "node", "port", "qIndex"]:
                match = re.search(rf"{key}\s+(\d+)", line)
                entry[key] = int(match.group(1)) if match else None

            data.append(entry)

    df = pd.DataFrame(data)
    df = df.dropna(subset=["node", "port", "qIndex"])
    df = df.sort_values(by=["node", "port", "qIndex", "Time"])

    # ✅ 计算 credit
    df["credit"] = df["fccl"] - df["fctbs"]
    df["credit"] = df["credit"].where(df["credit"].notna(), None)

    return df


# ------------------------
# 5. 绘图函数：按 node + port + qIndex 分组绘制
# ------------------------
def plot_metrics(df):
    metrics = ["remainBS", "credit", "inflight"]
    groups = df.groupby(["node", "port", "qIndex"])

    # ✅ 全局统一横坐标范围
    xmin = x_min if x_min is not None else df["Time"].min()
    xmax = x_max if x_max is not None else df["Time"].max()

    print(f"📊 全局横坐标范围: xmin={xmin}, xmax={xmax}")

    for metric in metrics:
        for (node, port, qIndex), group in groups:
            if metric not in group.columns or group[metric].isna().all():
                continue

            # ✅ 不进行纵坐标过滤
            plt.figure(figsize=(8, 5))
            plt.plot(
                group["Time"],
                group[metric],
                linestyle='-',      # 连线
                linewidth=1.8,      # 稍粗一点
            )

            plt.xlabel("Time")
            plt.ylabel(metric)
            plt.title(f"{metric} over Time (node={node}, port={port}, qIndex={qIndex})")

            plt.xlim(xmin, xmax)
            plt.ylim(bottom=0)
            plt.grid(alpha=0.3)
            plt.tight_layout()

            output_file = os.path.join(
                plot_dir,
                f"{metric}_node{node}_port{port}_q{qIndex}.png"
            )
            plt.savefig(output_file, dpi=300, bbox_inches="tight")
            plt.close()
            print(f"✅ 图像已保存: {output_file}")


# ------------------------
# 6. 主流程
# ------------------------
if __name__ == "__main__":
    df = parse_simulation_log(log_path)
    if df.empty:
        print("⚠️ 没有匹配到任何数据，请检查日志格式。")
    else:
        print(f"✅ 成功解析到 {len(df)} 条记录，共 {df['node'].nunique()} 个节点。")
        plot_metrics(df)
