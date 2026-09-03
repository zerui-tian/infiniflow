#!/bin/bash
set -e

############################################
# 20-parameter-sweeping: qmin/qmax parameter sweep
# - 4 flow traces (W3/W4 x load40/load80)
# - 3 link delays (500ns, 1us, 2us)
# - qmin/qmax sweep:
#   qmin=4,6,...,34; qmax step=2; qmax<=36
############################################

SCRIPT_PATH="$(readlink -f "$0")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"
CONFIG_FILE="$SCRIPT_DIR/../config.sh"
source $CONFIG_FILE

HEADER_FILE="$NS3/src/point-to-point/model/global-config.h"

Q_CNT_VALUE=1024
F_CNT_VALUE=1024
P_CNT_VALUE=64

MODEL_NAMES=("W3" "W4")
LOAD_NUMS=("40" "80")

DELAY_LABELS=("500ns" "1us" "2us")
DELAY_NS_LIST=(500 1000 2000)

TOPOLOGY_FILES=(
    "examples/SharedCredit-FC/20-parameter-sweeping/20-parameter-sweeping_topology_500ns.txt"
    "examples/SharedCredit-FC/20-parameter-sweeping/20-parameter-sweeping_topology_1us.txt"
    "examples/SharedCredit-FC/20-parameter-sweeping/20-parameter-sweeping_topology_2us.txt"
)

if [[ "$#" -gt 0 ]]; then
    RUN_DELAY_LABELS=("$@")
else
    RUN_DELAY_LABELS=("${DELAY_LABELS[@]}")
fi

QMIN_MIN=4
QMAX_MAX=36
Q_STEP=2

QLEN_DUMP_INTERVAL_LOAD40=50000
QLEN_MON_INTERVAL_LOAD40=5000
QLEN_MON_END_LOAD40=5000000

QLEN_DUMP_INTERVAL_LOAD80=30000
QLEN_MON_INTERVAL_LOAD80=3000
QLEN_MON_END_LOAD80=3000000

base_dir="$EXAMPLE_DIR/20-parameter-sweeping"
config_dir="$base_dir/20-parameter-sweeping_config_temp"
flow_dir="$base_dir/flow"
output_dir="$OUTPUT/20-parameter-sweeping/infiniflow"
config_file="$config_dir/config-parameter-sweeping.txt"

get_delay_index() {
    local target_label=$1

    for delay_idx in "${!DELAY_LABELS[@]}"; do
        if [[ "${DELAY_LABELS[$delay_idx]}" == "$target_label" ]]; then
            echo "$delay_idx"
            return 0
        fi
    done

    return 1
}

mkdir -p "$output_dir/fct" "$output_dir/out" "$output_dir/qlen"

cd "$NS3" || { echo "Failed to enter NS3 directory"; exit 1; }

sed -i "s/#define Q_CNT [0-9]*/#define Q_CNT $Q_CNT_VALUE/" "$HEADER_FILE"
sed -i "s/#define F_CNT [0-9]*/#define F_CNT $F_CNT_VALUE/" "$HEADER_FILE"
sed -i "s/#define P_CNT [0-9]*/#define P_CNT $P_CNT_VALUE/" "$HEADER_FILE"
sed -i "s/#define OUTPUT_1 [0-9]*/#define OUTPUT_1 0/" "$HEADER_FILE"

./waf || { echo "Compilation failed"; exit 1; }

total_runs=0
completed_runs=0

for model in "${MODEL_NAMES[@]}"; do
    for load in "${LOAD_NUMS[@]}"; do
        flow_file="$flow_dir/${model}_load${load}.txt"
        if [[ ! -f "$flow_file" ]]; then
            echo "Warning: flow file not found: $flow_file"
            continue
        fi

        if [[ "$load" == "40" ]]; then
            qlen_dump_interval=$QLEN_DUMP_INTERVAL_LOAD40
            qlen_mon_interval=$QLEN_MON_INTERVAL_LOAD40
            qlen_mon_end=$QLEN_MON_END_LOAD40
        else
            qlen_dump_interval=$QLEN_DUMP_INTERVAL_LOAD80
            qlen_mon_interval=$QLEN_MON_INTERVAL_LOAD80
            qlen_mon_end=$QLEN_MON_END_LOAD80
        fi

        trace_tag="${model}_load${load}"

        for selected_delay_label in "${RUN_DELAY_LABELS[@]}"; do
            if ! delay_idx="$(get_delay_index "$selected_delay_label")"; then
                echo "Unknown delay label: $selected_delay_label"
                echo "Valid delay labels: ${DELAY_LABELS[*]}"
                exit 1
            fi

            delay_label="${DELAY_LABELS[$delay_idx]}"
            delay_ns="${DELAY_NS_LIST[$delay_idx]}"
            topology_file="${TOPOLOGY_FILES[$delay_idx]}"

            mkdir -p "$output_dir/fct/${delay_label}" \
                     "$output_dir/out/${delay_label}" \
                     "$output_dir/qlen/${delay_label}"

            for ((qmin = QMIN_MIN; qmin < QMAX_MAX; qmin += Q_STEP)); do
                for ((qmax = qmin + Q_STEP; qmax <= QMAX_MAX; qmax += Q_STEP)); do
                    total_runs=$((total_runs + 1))
                    run_tag="${trace_tag}_delay${delay_label}_qmin${qmin}_qmax${qmax}"

                    echo "=================================="
                    echo "Running: $run_tag"
                    echo "  topology=$topology_file"
                    echo "  delay=${delay_ns}ns"
                    echo "  q_step=$Q_STEP qmax_max=$QMAX_MAX"
                    echo "  qmin=$qmin qmax=$qmax"
                    echo "=================================="

                    {
                        cat "$config_dir/config-parameter-sweeping-common.txt"
                        echo "TOPOLOGY_FILE $topology_file"
                        echo "FLOW_FILE $flow_file"
                        echo "FCT_OUTPUT_FILE $output_dir/fct/${delay_label}/${trace_tag}_qmin${qmin}_qmax${qmax}.txt"
                        echo "QLEN_MON_FILE $output_dir/qlen/${delay_label}/${trace_tag}_qmin${qmin}_qmax${qmax}.txt"
                        echo "QLEN_DUMP_INTERVAL $qlen_dump_interval"
                        echo "QLEN_MON_INTERVAL $qlen_mon_interval"
                        echo "QLEN_MON_END 0"
                        echo "QLEN_MON_END $qlen_mon_end"
                        echo "QMIN $qmin"
                        echo "QMAX $qmax"
                    } > "$config_file"

                    ./waf --run "20-parameter-sweeping --conf=$config_file" \
                        > "$output_dir/out/${delay_label}/${trace_tag}_qmin${qmin}_qmax${qmax}.out" || {
                            echo "Experiment failed: $run_tag"
                            exit 1
                        }

                    completed_runs=$((completed_runs + 1))
                    echo "Completed ($completed_runs/$total_runs): $run_tag"
                    echo "----------------------------------"
                done
            done
        done
    done
done

echo "##################################"
echo "#      ALL EXPERIMENTS DONE      #"
echo "#  completed: $completed_runs    #"
echo "##################################"
