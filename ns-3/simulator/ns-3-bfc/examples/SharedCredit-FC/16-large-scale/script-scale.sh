#!/bin/bash

# 加载配置文件
SCRIPT_PATH="$(readlink -f "$0")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

CONFIG_FILE="$SCRIPT_DIR/../config.sh"
source $CONFIG_FILE

# 定义模型名称和负载数组
modelNames=("W3" "W4")

loadNums=("40" "80") # 4

QLEN_DUMP_INTERVAL_LIST=(
    50000 50000 50000 30000 30000 30000
)

QLEN_MON_INTERVAL_LIST=(
    5000 5000 5000 3000 3000 3000
)

QLEN_MON_END_LIST=(
    5000000 5000000 5000000 3000000 3000000 3000000
)

# 基础路径设置
base_dir="$EXAMPLE_DIR/16-large-scale"
config_dir="$base_dir/16-large-scale_config_temp"
flow_dir="$base_dir/flows"
output_dir="$SIMULATION_ROOT/ns-3/simulator/result/data/16-large-scale/bfc"
config_file="$config_dir/config-common.txt"

mkdir -p "$output_dir"
mkdir -p "$output_dir/fct"
mkdir -p "$output_dir/output"
mkdir -p "$output_dir/qlen"
# 切换到NS3目录
cd "$NS3" || { echo "Failed to enter NS3 directory"; exit 1; }

HEADER_FILE="$NS3/src/point-to-point/model/global-config.h"

# 修改头文件中的宏定义
sed -i "s/#define Q_CNT [0-9]*/#define Q_CNT 32/" "$HEADER_FILE"
sed -i "s/#define P_CNT [0-9]*/#define P_CNT 64/" "$HEADER_FILE"
sed -i "s/#define F_CNT [0-9]*/#define F_CNT 1024/" "$HEADER_FILE"
sed -i "s/#define OUTPUT_1 [0-9]*/#define OUTPUT_1 0/" "$HEADER_FILE"
# 编译NS3项目
./waf || { echo "Compilation failed"; exit 1; }

# 主循环：遍历所有模型和负载组合
for model in "${modelNames[@]}"; do
    echo "=================================="
    echo "Processing model: $model"
    echo "=================================="
    
    for load in "${loadNums[@]}"; do
        # 检查流量配置文件是否存在
        flow_file="$flow_dir/${model}_load${load}.txt"
        if [ ! -f "$flow_file" ]; then
            echo "Warning: Flow file $flow_file not found, skipping..."
            continue
        fi

        echo "Running experiment: ${model}_load${load}"
        QLEN_DUMP_INTERVAL=${QLEN_DUMP_INTERVAL_LIST[$trace_idx]}
        QLEN_MON_INTERVAL=${QLEN_MON_INTERVAL_LIST[$trace_idx]}
        QLEN_MON_END=${QLEN_MON_END_LIST[$trace_idx]}
        # 生成配置文件内容
        {
            cat "$config_dir/config-scale-common.txt"
            echo "FLOW_FILE $flow_file"
            echo "FCT_OUTPUT_FILE $output_dir/fct/${model}_load${load}.txt"
            echo "QLEN_MON_FILE $output_dir/qlen/${model}_load${load}.txt"
            echo "QLEN_DUMP_INTERVAL $QLEN_DUMP_INTERVAL"
            echo "QLEN_MON_INTERVAL $QLEN_MON_INTERVAL"
            echo "QLEN_MON_END 0"
            echo "QLEN_MON_END $QLEN_MON_END"
        } > "$config_file"

        # 运行实验
        ./waf --run "16-large-scale --conf=$config_file --windowCheck=1" > "$output_dir/output/${model}_load${load}.out"|| {
            echo "Experiment ${model}_load${load} failed!"
            continue
        }

        echo "Experiment ${model}_load${load} completed successfully"
        echo "----------------------------------"
    done
done

echo "##################################"
echo "#      ALL EXPERIMENTS DONE      #"
echo "##################################"