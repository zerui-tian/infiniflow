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

############################################
# Traces (load changes)
############################################
TRACE_LIST=(
    # "W4_load40"
    "W4_load80"
)

############################################
# Fixed parameters
############################################
PCNT=64
FCNT=4100
VC_CNT=4096
BUFFER_SIZE=6400      # 单位：KB（写 config 时 *1000）

############################################
# qmin / qmax parameters
############################################
QMIN_LIST=(2 3 4 5)
QDELTA_LIST=(2 3 4 5)   # qmax = qmin + delta

############################################
# Batch run
############################################
for TRACE_NAME in "${TRACE_LIST[@]}"; do

    echo "=============================="
    echo " Running TRACE = $TRACE_NAME "
    echo "=============================="

    TRACE_OUT_DIR="$output_dir/$TRACE_NAME"
    TRACE_FCT_DIR="$TRACE_OUT_DIR/fct/qmin_qmax"
    mkdir -p "$TRACE_FCT_DIR" "$TRACE_OUT_DIR/out/qmin_qmax"

    ############################################
    # Enter ns-3 and build once per trace
    ############################################
    cd "$NS3" || exit 1

    sed -i "s/#define Q_CNT [0-9]*/#define Q_CNT $VC_CNT/" "$HEADER_FILE"
    sed -i "s/#define F_CNT [0-9]*/#define F_CNT $FCNT/" "$HEADER_FILE"
    sed -i "s/#define P_CNT [0-9]*/#define P_CNT $PCNT/" "$HEADER_FILE"
    sed -i "s/#define OUTPUT [0-9]*/#define OUTPUT 0/" "$HEADER_FILE"
    ./waf || { echo "Compilation failed"; exit 1; }

    ############################################
    # Flow file (VC fixed)
    ############################################
    FLOW_FILE="$flow_dir/${TRACE_NAME}/${TRACE_NAME}_${VC_CNT}.txt"

    if [[ ! -f "$FLOW_FILE" ]]; then
        echo "Flow file not found: $FLOW_FILE"
        exit 1
    fi

    ############################################
    # qmin / qmax sweep
    ############################################
    for qmin in "${QMIN_LIST[@]}"; do
        for delta in "${QDELTA_LIST[@]}"; do

            qmax=$((qmin + delta))

            echo "TRACE=$TRACE_NAME | qmin=$qmin | qmax=$qmax"

            ############################################
            # Output files
            ############################################
            FCT_FILE="$TRACE_FCT_DIR/qmin${qmin}_qmax${qmax}.txt"
            STDOUT_FILE="$TRACE_OUT_DIR/out/qmin_qmax/qmin${qmin}_qmax${qmax}.out"

            ############################################
            # Generate config
            ############################################
            {
                cat "$config_dir/config_template.txt"
                echo "BUFFER_SIZE $((BUFFER_SIZE * 1000))"
                echo "FLOW_FILE $FLOW_FILE"
                echo "FCT_OUTPUT_FILE $FCT_FILE"
                echo "QMIN $qmin"
                echo "QMAX $qmax"
            } > "$config_file"

            ############################################
            # Run experiment
            ############################################
            ./waf --run "15-evaluation --conf=$config_file" \
                > "$STDOUT_FILE" || {
                    echo "Experiment failed:"
                    echo "  TRACE=$TRACE_NAME"
                    echo "  qmin=$qmin"
                    echo "  qmax=$qmax"
                    exit 1
                }

        done
    done
done

echo "##################################"
echo "#      ALL EXPERIMENTS DONE      #"
echo "##################################"
