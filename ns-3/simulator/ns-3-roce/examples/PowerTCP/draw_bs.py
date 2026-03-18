import re
import pandas as pd
import matplotlib.pyplot as plt
import os

# ------------------------
# 1. 日志文件路径（修改这里）
# ------------------------
log_path = "dump_test/evaluation-NoAlgorithm.out"  # 你的日志文件路径

# ------------------------
# 2. 正则匹配“switch”行
# ------------------------
pattern = re.compile(
    r"switch\s+(?P<switch>\d+)\s+port\s+(?P<port>\d+)\s+qIndex\s+(?P<qIndex>\d+)\s+bs\s+(?P<bs>\d+)\s+time\s+(?P<time>\d+)"
)

data = []

with open(log_path, "r") as f:
    for line in f:
        match = pattern.search(line)
        if match:
            data.append({
                "switch": int(match.group("switch")),
                "port": int(match.group("port")),
                "qIndex": int(match.group("qIndex")),
                "bs": int(match.group("bs")),
                "time": int(match.group("time")),
            })

if not data:
    print("⚠️ 未匹配到任何 switch 数据，请检查日志路径或格式。")
    exit()

df = pd.DataFrame(data)
print(f"✅ 共提取 {len(df)} 条 switch 记录")

# ------------------------
# 3. 图像保存目录
# ------------------------
plot_dir = "plots_switch_waveform"
os.makedirs(plot_dir, exist_ok=True)

# ------------------------
# 4. 分组绘制波形图
# ------------------------
grouped = df.groupby(["switch", "port", "qIndex"])

for (sw, port, qidx), group in grouped:
    group = group.sort_values("time")

    plt.figure(figsize=(8, 4))
    
    # ✅ 绘制波形图 —— 无散点，平滑曲线
    plt.plot(group["time"], group["bs"], color="tab:blue", linewidth=1.5)
    
    # 视觉增强
    plt.fill_between(group["time"], group["bs"], color="tab:blue", alpha=0.15)
    
    # 自动纵坐标
    ymin, ymax = group["bs"].min(), group["bs"].max()
    margin = (ymax - ymin) * 0.1 if ymax > ymin else 1
    plt.ylim(ymin - margin, ymax + margin)
    
    plt.title(f"Switch {sw} - Port {port} - qIndex {qidx}", fontsize=12)
    plt.xlabel("Time", fontsize=10)
    plt.ylabel("Buffer Size (bs)", fontsize=10)
    plt.grid(True, linestyle="--", alpha=0.6)
    plt.tight_layout()
    
    # 保存文件
    filename = f"switch_{sw}_port_{port}_q{qidx}_wave.png"
    plt.savefig(os.path.join(plot_dir, filename))
    plt.close()

print(f"✅ 所有波形图已生成，保存在文件夹：{plot_dir}")
