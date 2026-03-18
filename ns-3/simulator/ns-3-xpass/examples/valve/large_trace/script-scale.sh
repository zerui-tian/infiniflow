#!/bin/bash

# 加载配置文件
source ../config.sh

# 定义模型名称和负载数组
modelNames=("websearch" "hadoop" "datamining")
loadNums=("30" "50" "80")

# 基础路径设置
base_dir="$EXAMPLE_DIR/large_scale"
config_dir="$base_dir/large_scale_config_temp"
flow_dir="$base_dir/flow"
output_dir="$base_dir/monitor_output"
config_file="$config_dir/config-scale.txt"

# 切换到NS3目录
cd "$NS3" || { echo "Failed to enter NS3 directory"; exit 1; }

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
        
        # 生成配置文件内容
        {
            cat "$config_dir/config-scale-common.txt"
            echo "FLOW_FILE $flow_file"
            echo "FCT_OUTPUT_FILE $output_dir/fct/${model}_load${load}/valve.txt"
            echo "PFC_OUTPUT_FILE $output_dir/pfc/${model}_load${load}/valve.txt"
        } > "$config_file"

        # 运行实验
        ./waf --run "large_scale --conf=$config_file" > "$base_dir/large_scale_output.txt"|| {
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