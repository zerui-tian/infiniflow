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
flow_base_dir="$base_dir/flows"
output_base_dir="$OUTPUT/15-evaluation"
config_file="$config_dir/config.txt"

HEADER_FILE="$NS3/src/point-to-point/model/global-config.h"

############################################
# Fixed parameters
############################################
BUFFER_SIZE=6400          # total buffer (KB)
PCNT=64
FCNT=8200

############################################
# Experiment parameters
############################################
# TRACE_LIST=(W4_load20 W4_load40 W4_load60 W4_load80 W4_load90)
TRACE_LIST=(W4_load40 W4_load80)
VC_CNT_LIST=(8 16 32 64 128 256 512 1024 2048 4096 8192)

############################################
# Build NS-3 once (VC will be updated later)
############################################
cd "$NS3" || exit 1
sed -i "s/#define F_CNT [0-9]*/#define F_CNT $FCNT/" "$HEADER_FILE"
sed -i "s/#define P_CNT [0-9]*/#define P_CNT $PCNT/" "$HEADER_FILE"
sed -i "s/#define OUTPUT_1 [0-9]*/#define OUTPUT_1 0/" "$HEADER_FILE"

############################################
# Batch run
############################################
for TRACE in "${TRACE_LIST[@]}"; do
    echo "======================================"
    echo "[TRACE] $TRACE"
    echo "======================================"

    FLOW_DIR="$flow_base_dir/$TRACE"
    TRACE_OUTPUT_DIR="$output_base_dir/$TRACE"
    FCT_DIR="$TRACE_OUTPUT_DIR/fct/vcs"
    OUT_DIR="$TRACE_OUTPUT_DIR/out/vcs"

    mkdir -p "$FCT_DIR" "$OUT_DIR"

    for VC_CNT in "${VC_CNT_LIST[@]}"; do
        ############################################
        # Update VC macro
        ############################################
        sed -i "s/#define Q_CNT [0-9]*/#define Q_CNT $VC_CNT/" "$HEADER_FILE"
        ./waf || { echo "Compilation failed"; exit 1; }

        ############################################
        # Flow file
        ############################################
        FLOW_FILE="$FLOW_DIR/${TRACE}_${VC_CNT}.txt"

        if [[ ! -f "$FLOW_FILE" ]]; then
            echo "[ERROR] Flow file not found: $FLOW_FILE"
            exit 1
        fi

        ############################################
        # Output files
        ############################################
        FCT_FILE="$FCT_DIR/buf${BUFFER_SIZE}_vc${VC_CNT}.txt"
        STDOUT_FILE="$OUT_DIR/buf${BUFFER_SIZE}_vc${VC_CNT}.out"

        echo "[RUN] TRACE=$TRACE | BUFFER=$BUFFER_SIZE | VC=$VC_CNT"

        ############################################
        # Generate config
        ############################################
        {
            cat "$config_dir/config_template.txt"
            echo "BUFFER_SIZE $((BUFFER_SIZE * 1000))"
            echo "FLOW_FILE $FLOW_FILE"
            echo "FCT_OUTPUT_FILE $FCT_FILE"
        } > "$config_file"

        ############################################
        # Run experiment
        ############################################
        ./waf --run "15-evaluation --conf=$config_file" \
            > "$STDOUT_FILE" || {
                echo "[FAIL] TRACE=$TRACE | VC=$VC_CNT"
                exit 1
            }

    done
done

echo "##################################"
echo "#      ALL EXPERIMENTS DONE      #"
echo "##################################"
