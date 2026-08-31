#!/usr/bin/env bash

set -euo pipefail

SCRIPT_PATH="$(readlink -f "$0")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

CONFIG_FILE="$SCRIPT_DIR/../ns-3-infiniflow-mmf/examples/SharedCredit-FC/config.sh"
source "$CONFIG_FILE"

SENSITIVITY_CONFIG="$EXAMPLE_DIR/19-sensitivity/19-sensitivity_config.txt"
GLOBAL_CONFIG="$NS3/src/point-to-point/model/global-config.h"
RESULT_DIR="$OUTPUT/19-sensitivity"

mkdir -p \
    "$OUTPUT/19-sensitivity" \
    "$EXAMPLE_DIR/19-sensitivity/monitor_output"

# 修改编译配置。
sed -i -E \
    's|^[[:space:]]*#define[[:space:]]+OUTPUT_1[[:space:]]+[0-9]+|#define OUTPUT_1 0|' \
    "$GLOBAL_CONFIG"

cd "$NS3"

# OUTPUT_1 修改后编译一次。
./waf

QMIN_QMAX_PAIRS=(
    "2 4"
    "8 16"
    "28 36"
)

for pair in "${QMIN_QMAX_PAIRS[@]}"; do
    read -r qmin qmax <<< "$pair"

    throughput_file="$RESULT_DIR/flowthroughput_${qmin}_${qmax}.txt"
    output_file="$RESULT_DIR/19-sensitivity_${qmin}_${qmax}.out"

    sed -i -E \
        -e "s|^[[:space:]]*QMIN[[:space:]]+.*$|QMIN ${qmin}|" \
        -e "s|^[[:space:]]*QMAX[[:space:]]+.*$|QMAX ${qmax}|" \
        -e "s|^[[:space:]]*FLOWTHROUGHPUT_MON_FILE[[:space:]]+.*$|FLOWTHROUGHPUT_MON_FILE ${throughput_file}|" \
        "$SENSITIVITY_CONFIG"

    echo "##################################"
    echo "# Running QMIN=${qmin}, QMAX=${qmax}"
    echo "##################################"

    ./waf --run "19-sensitivity --conf=$SENSITIVITY_CONFIG" \
        > "$output_file" 2>&1
done

echo "##################################"
echo "#      EXPERIMENTS FINISHED      #"
echo "##################################"