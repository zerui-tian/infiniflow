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
base_dir="$EXAMPLE_DIR/15-evaluation"
config_dir="$base_dir/15-evaluation_config_temp"
output_dir="$OUTPUT/15-evaluation"
flow_dir="$base_dir/flows"
config_file="$config_dir/config.txt"

HEADER_FILE="$NS3/src/point-to-point/model/global-config.h"

TRACE_LIST=(
    "W4_load40"
    "W4_load80"
)

############################################
# Fixed parameters
############################################
PCNT=64
FCNT=4100
VC_CNT=4096          # ★ VC 固定为 4096

############################################
# Experiment parameters
############################################
BUFFER_SIZE_LIST=(1600 1920 2240 2560 2880 3200
    3520 3840 4480 4800 5120 5760 6080 6400)

############################################
# Batch run
############################################
for TRACE_NAME in "${TRACE_LIST[@]}"; do

    echo "=============================="
    echo " Running TRACE = $TRACE_NAME "
    echo "=============================="

    TRACE_OUT_DIR="$output_dir/$TRACE_NAME"
    TRACE_FCT_DIR="$TRACE_OUT_DIR/fct/buffer/"
    mkdir -p "$TRACE_FCT_DIR"
    mkdir -p "$TRACE_OUT_DIR/out/buffer/"
    for buffer_mul in "${BUFFER_SIZE_LIST[@]}"; do
        cd "$NS3" || exit 1

        ############################################
        # Modify global macros
        ############################################
        sed -i "s/#define Q_CNT [0-9]*/#define Q_CNT $VC_CNT/" "$HEADER_FILE"
        sed -i "s/#define F_CNT [0-9]*/#define F_CNT $FCNT/" "$HEADER_FILE"
        sed -i "s/#define P_CNT [0-9]*/#define P_CNT $PCNT/" "$HEADER_FILE"
        sed -i "s/#define OUTPUT_1 [0-9]*/#define OUTPUT_1 0/" "$HEADER_FILE"
        ./waf || { echo "Compilation failed"; exit 1; }

        ############################################
        # Flow file (trace + fixed VC)
        ############################################
        FLOW_FILE="$flow_dir/${TRACE_NAME}/${TRACE_NAME}_${VC_CNT}.txt"

        if [[ ! -f "$FLOW_FILE" ]]; then
            echo "Flow file not found: $FLOW_FILE"
            exit 1
        fi

        ############################################
        # Output files
        ############################################
        FCT_FILE="$TRACE_FCT_DIR/buf${buffer_mul}_vc${VC_CNT}.txt"
        STDOUT_FILE="$TRACE_OUT_DIR/out/buffer/buf${buffer_mul}_vc${VC_CNT}.out"

        echo "TRACE=$TRACE_NAME | BUFFER=$buffer_mul | VC=$VC_CNT"

        ############################################
        # Generate config
        ############################################
        {
            cat "$config_dir/config_template.txt"
            echo "BUFFER_SIZE $((buffer_mul * 1000))"
            echo "FLOW_FILE $FLOW_FILE"
            echo "FCT_OUTPUT_FILE $FCT_FILE"
        } > "$config_file"

        ############################################
        # Run experiment
        ############################################
        ./waf --run "15-evaluation --conf=$config_file" \
            > "$STDOUT_FILE" || {
                echo "Experiment failed:"
                echo "  TRACE=$TRACE_NAME"
                echo "  BUFFER=$buffer_mul"
                echo "  VC=$VC_CNT"
                exit 1
            }

    done
done

echo "##################################"
echo "#      ALL EXPERIMENTS DONE      #"
echo "##################################"
