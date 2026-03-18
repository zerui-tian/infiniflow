#!/bin/bash

SCRIPT_PATH="$(readlink -f "$0")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

CONFIG_FILE="$SCRIPT_DIR/../config.sh"
source $CONFIG_FILE

HEADER_FILE="$NS3/src/point-to-point/model/global-config.h"

Q_CNT_VALUE=1000
F_CNT_VALUE=1000
P_CNT_VALUE=64
QLEN_DUMP_INTERVAL_LIST=(
    50000 50000 50000 30000 30000 30000
)

QLEN_MON_INTERVAL_LIST=(
    5000 5000 5000 3000 3000 3000
)

QLEN_MON_END_LIST=(
    5000000 5000000 5000000 3000000 3000000 3000000
)

sed -i "s/#define Q_CNT [0-9]*/#define Q_CNT $Q_CNT_VALUE/" "$HEADER_FILE"
sed -i "s/#define F_CNT [0-9]*/#define F_CNT $F_CNT_VALUE/" "$HEADER_FILE"
sed -i "s/#define P_CNT [0-9]*/#define P_CNT $P_CNT_VALUE/" "$HEADER_FILE"
sed -i "s/#define OUTPUT_1 [0-9]*/#define OUTPUT_1 0/" "$HEADER_FILE"

modelNames=("W3" "W4")
loadNums=("40" "80")

base_dir="$EXAMPLE_DIR/7-batch-processing"
config_dir="$base_dir/7-batch-processing_config_temp"
flow_dir="$base_dir/flow"
output_dir="$base_dir/monitor_output"
config_file="$config_dir/config-scale.txt"

mkdir -p "$output_dir"
mkdir -p "$output_dir/fct"
mkdir -p "$output_dir/output"
mkdir -p "$output_dir/qlen"

cd "$NS3" || { echo "Failed to enter NS3 directory"; exit 1; }

./waf || { echo "Compilation failed"; exit 1; }

trace_idx=0

for model in "${modelNames[@]}"; do
    echo "=================================="
    echo "Processing model: $model"
    echo "=================================="
    
    for load in "${loadNums[@]}"; do
        flow_file="$flow_dir/${model}_load${load}.txt"
        if [ ! -f "$flow_file" ]; then
            echo "Warning: Flow file $flow_file not found, skipping..."
            continue
        fi

        echo "Running experiment: ${model}_load${load}"
        QLEN_DUMP_INTERVAL=${QLEN_DUMP_INTERVAL_LIST[$trace_idx]}
        QLEN_MON_INTERVAL=${QLEN_MON_INTERVAL_LIST[$trace_idx]}
        QLEN_MON_END=${QLEN_MON_END_LIST[$trace_idx]}

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

        ./waf --run "7-batch-processing --conf=$config_file" > "$output_dir/output/${model}_load${load}.out"|| {
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