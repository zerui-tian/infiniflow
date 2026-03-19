#!/bin/bash
set -e

############################################
# Load global config
############################################
SCRIPT_PATH="$(readlink -f "$0")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

CONFIG_FILE="$SCRIPT_DIR/../config.sh"
source $CONFIG_FILE
############################################
# Base paths
############################################
base_dir="$EXAMPLE_DIR/3-motivation"
config_dir="$base_dir/3-motivation_config_temp"
output_dir="$base_dir/monitor_output"
flow_dir="$base_dir/flows"                 # flow 文件目录
config_file="$config_dir/config_hpcc.txt"

HEADER_FILE="$NS3/src/point-to-point/model/global-config.h"
# TRACE_NAME="flow_diffusion2"
TRACE_NAME="W4_load160-sigma2"
PCNT=64
FCNT=1025
mkdir -p "$output_dir"
mkdir -p "$OUTPUT/3-motivation"

############################################
# Experiment parameters
############################################
BUFFER_SIZE_LIST=(20 50 100 200 500 1000)     # 外层：buffer    # 外层：buffer
VC_CNT_LIST=(8 16 32 64 128 256 512 1024)               # 内层：VC / qCnt

############################################
# Batch run
############################################
for buffer_mul in "${BUFFER_SIZE_LIST[@]}"; do
    ############################################
    # Modify global macro: Q_CNT = buffer
    ############################################
    

    ############################################
    # Build NS-3
    ############################################
    
    for vc_cnt in "${VC_CNT_LIST[@]}"; do
        cd "$NS3" || exit 1
        sed -i "s/#define Q_CNT [0-9]*/#define Q_CNT $vc_cnt/" "$HEADER_FILE"
        sed -i "s/#define F_CNT [0-9]*/#define F_CNT $FCNT/" "$HEADER_FILE"
        sed -i "s/#define P_CNT [0-9]*/#define P_CNT $PCNT/" "$HEADER_FILE"
        ./waf || { echo "Compilation failed"; exit 1; }
        ############################################
        # Flow file (按 VC 选择)
        ############################################
        FLOW_FILE="$flow_dir/${TRACE_NAME}_${vc_cnt}.txt"

        if [[ ! -f "$FLOW_FILE" ]]; then
            echo "Flow file not found: $FLOW_FILE"
            exit 1
        fi

        ############################################
        # Output files（名字包含 buffer + VC）
        ############################################
        FCT_FILE="$OUTPUT/3-motivation/buf${buffer_mul}_vc${vc_cnt}_hpcc.txt"
        STDOUT_FILE="$output_dir/buf${buffer_mul}_vc${vc_cnt}_hpcc.out"

        echo "BUFFER=$buffer_mul | VC=$vc_cnt"

        ############################################
        # Generate config
        ############################################
        {
            cat "$config_dir/config_template_hpcc.txt"
            echo "BUFFER_SIZE $((buffer_mul*1000000))"
            echo "FLOW_FILE $FLOW_FILE"
            echo "FCT_OUTPUT_FILE $FCT_FILE"
        } > "$config_file"

        ############################################
        # Run experiment
        ############################################
        ./waf --run "3-motivation --conf=$config_file --windowCheck=1" \
            > "$STDOUT_FILE" || {
                echo "Experiment failed:"
                echo "  BUFFER=$buffer_mul"
                echo "  VC=$vc_cnt"
                exit 1
            }
    done
done

echo "##################################"
echo "#      ALL EXPERIMENTS DONE      #"
echo "##################################"
