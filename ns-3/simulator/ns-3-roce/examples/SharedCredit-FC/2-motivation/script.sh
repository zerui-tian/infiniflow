#!/bin/bash
set -e

#######################################
# Basic environment
#######################################
SCRIPT_PATH="$(readlink -f "$0")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

CONFIG_FILE="$SCRIPT_DIR/../config.sh"
source $CONFIG_FILE

NS3_DIR="$NS3"
EXAMPLE_DIR="$EXAMPLE_DIR/2-motivation"

CONFIG_FILES=(
    "2-motivation_config_dcqcn.txt"
    "2-motivation_config_hpcc.txt"
    "2-motivation_config_ns.txt"
    "2-motivation_config.txt"
)
OUTPUT_DIR="$EXAMPLE_DIR/monitor_output"

mkdir -p "$OUTPUT_DIR"

HEADER_FILE="$NS3/src/point-to-point/model/global-config.h"

# 修改头文件中的宏定义
sed -i "s/#define Q_CNT [0-9]*/#define Q_CNT 8/" "$HEADER_FILE"
sed -i "s/#define P_CNT [0-9]*/#define P_CNT 41/" "$HEADER_FILE"
sed -i "s/#define F_CNT [0-9]*/#define F_CNT 128/" "$HEADER_FILE"
sed -i "s/#define OUTPUT_1 [0-9]*/#define OUTPUT_1 0/" "$HEADER_FILE"

cd "$NS3_DIR"

#######################################
# Build once
#######################################
./waf

for cfg in "${CONFIG_FILES[@]}"; do
    configFile="$EXAMPLE_DIR/$cfg"
    outputFile="$OUTPUT_DIR/${cfg%.txt}_pfc_output.txt"

    # 判断协议类型
    if [[ "$cfg" == *"dcqcn"* ]]; then
        windowCheck=0
        rate_file="sendingrate_dcqcn.txt"
    elif [[ "$cfg" == *"hpcc"* ]]; then
        windowCheck=1
        rate_file="sendingrate_hpcc.txt"
    elif [[ "$cfg" == *"ns"* ]]; then
        windowCheck=1
        rate_file="sendingrate_pfc_ns.txt"
    else
        windowCheck=1
        rate_file="sendingrate_pfc_alone.txt"
    fi

    FLOWRATE_PATH="$OUTPUT/2-motivation/$rate_file"

    # 修改 config 文件中的 FLOWRATE_MON_FILE
    sed -i "s|^FLOWRATE_MON_FILE .*|FLOWRATE_MON_FILE $FLOWRATE_PATH|" "$configFile"

    echo "===================================="
    echo "Running with config: $cfg"
    echo "windowCheck = $windowCheck"
    echo "FLOWRATE file = $FLOWRATE_PATH"
    echo "Output -> $outputFile"
    echo "===================================="

    ./waf --run "2-motivation --conf=$configFile --windowCheck=$windowCheck" > "$outputFile"
done

echo "##################################"
echo "#      ALL EXPERIMENTS FINISHED   #"
echo "##################################"
