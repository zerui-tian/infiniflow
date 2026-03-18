import re
import pandas as pd
import matplotlib.pyplot as plt
import os

# ------------------------
# 1. 主日志文件路径（波形数据）
# ------------------------
log_path = "dump_test/evaluation-NoAlgorithm.out"

# ------------------------
# 2. 事件文件路径（pause/resume 数据）
# ------------------------
event_path = "/home/pnic/pfc-ns3/simulator/ns-3.39/mix/pfc.txt"

# ------------------------
# 3. 从日志中提取 xon/xoff 值（同一行，找到第一个匹配就停止）
# ------------------------
xon, xoff = None, None
pattern_x = re.compile(r"xon\s+(\d+)\s+xoff\s+(\d+)")

with open(log_path, "r") as f:
    for line in f:
        m = pattern_x.search(line)
        if m:
            xon = int(m.group(1))
            xoff = int(m.group(2))
            break  # 找到第一个匹配就停止

if xon is None or xoff is None:
    print("⚠️ 未在日志文件中找到 xon/xoff（格式应为：'xon <num> xoff <num>'），使用默认值。")
    xon = 20000
    xoff = 80000
else:
    print(f"✅ 从日志文件第一个匹配行提取阈值：xon={xon}, xoff={xoff}")

# ------------------------
# 4. 读取波形日志
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
# 5. 读取事件文件（包含 qIndex）
# ------------------------
events = pd.read_csv(
    event_path,
    sep=r"\s+",
    header=None,
    names=["time", "switch", "is_host", "port", "type", "qIndex"]
)

print(f"✅ 共读取 {len(events)} 条事件记录")

# ------------------------
# 6. 输出目录
# ------------------------
plot_dir = "plots_switch_waveform_line"
os.makedirs(plot_dir, exist_ok=True)

# ------------------------
# 7. 按 (switch, port, qIndex) 分组绘制波形 + 标注事件
# ------------------------
grouped = df.groupby(["switch", "port", "qIndex"])

for (sw, port, qidx), group in grouped:
    if qidx == 0:
        continue  # 🚫 不关注 qIndex=0 的图

    group = group.sort_values("time")

    # ✅ 拉宽图像（原来 8x4 → 改为 12x4）
    plt.figure(figsize=(12, 4))
    plt.plot(group["time"], group["bs"], color="tab:blue", linewidth=1.5, label="Buffer size")
    plt.fill_between(group["time"], group["bs"], color="tab:blue", alpha=0.15)

    # ✅ 基准虚线
    plt.axhline(y=xoff, color="orange", linestyle="--", linewidth=1.2, label=f"xoff={xoff}")
    plt.axhline(y=xon, color="purple", linestyle="--", linewidth=1.2, label=f"xon={xon}")

    # 自动纵坐标范围
    ymin, ymax = group["bs"].min(), group["bs"].max()
    margin = (ymax - ymin) * 0.1 if ymax > ymin else 1
    plt.ylim(ymin - margin, ymax + margin)

    # ✅ 精确匹配相同 (switch, port, qIndex) 的事件
    ev_subset = events[
        (events["switch"] == sw)
        & (events["port"] == port)
        & (events["qIndex"] == qidx)
    ]

    # ✅ 定义事件图例句柄，确保始终存在六个图例项
    handles = {
        "recv pause": None,
        "recv resume": None,
        "send pause": None,
        "send resume": None,
    }

    for _, ev in ev_subset.iterrows():
        if ev["type"] == 0:  # recv pause
            line = plt.axvline(ev["time"], color="red", linestyle="-", linewidth=1.2, alpha=0.8)
            handles["recv pause"] = line
        elif ev["type"] == 1:  # recv resume
            line = plt.axvline(ev["time"], color="green", linestyle="-", linewidth=1.2, alpha=0.8)
            handles["recv resume"] = line
        elif ev["type"] == 2:  # send pause
            line = plt.axvline(ev["time"], color="red", linestyle="--", linewidth=1.2, alpha=0.8)
            handles["send pause"] = line
        elif ev["type"] == 3:  # send resume
            line = plt.axvline(ev["time"], color="green", linestyle="--", linewidth=1.2, alpha=0.8)
            handles["send resume"] = line

    plt.title(f"Switch {sw} - Port {port} - qIndex {qidx}", fontsize=12)
    plt.xlabel("Time", fontsize=10)
    plt.ylabel("Buffer Size (bs)", fontsize=10)
    plt.grid(True, linestyle="--", alpha=0.6)

    # ✅ 构建固定顺序的图例（确保6个）
    legend_handles = [
        plt.Line2D([], [], color="orange", linestyle="--", label=f"xoff={xoff}"),
        plt.Line2D([], [], color="purple", linestyle="--", label=f"xon={xon}"),
        plt.Line2D([], [], color="red", linestyle="-", label="recv pause"),
        plt.Line2D([], [], color="green", linestyle="-", label="recv resume"),
        plt.Line2D([], [], color="red", linestyle="--", label="send pause"),
        plt.Line2D([], [], color="green", linestyle="--", label="send resume"),
    ]

    # ✅ 图例放到图外（右侧）
    plt.legend(
        handles=legend_handles,
        fontsize=9,
        loc="center left",
        bbox_to_anchor=(1.02, 0.5),
        framealpha=0.8
    )

    plt.tight_layout(rect=[0, 0, 0.85, 1])  # 留出右侧图例空间

    # 保存图片
    filename = f"switch_{sw}_port_{port}_q{qidx}_wave.png"
    plt.savefig(os.path.join(plot_dir, filename), bbox_inches="tight")
    plt.close()

print(f"✅ 所有波形图已生成，保存在文件夹：{plot_dir}")
