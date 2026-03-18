#!/bin/bash
set -e

SCRIPT_PATH="$(readlink -f "$0")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

CONFIG_FILE="$SCRIPT_DIR/../config.sh"
source $CONFIG_FILE

NS3_DIR="$NS3"
EXAMPLE_DIR="$NS3/examples/SharedCredit-FC/2-motivation"

CONFIG_FILES=(
    "2-motivation_config_ns.txt"
    "2-motivation_config.txt"
)

OUTPUT_DIR="$EXAMPLE_DIR/monitor_output"

mkdir -p "$OUTPUT_DIR"

cd "$NS3_DIR"

#######################################
# Build once
#######################################
./waf

OUTPUT_ABS="$(readlink -f "$OUTPUT")"

for cfg in "${CONFIG_FILES[@]}"; do
    configFile="$EXAMPLE_DIR/$cfg"
    
    echo "Processing $cfg..."

    echo "Original FLOWRATE_MON_FILE line:"
    grep "^FLOWRATE_MON_FILE" "$configFile" || echo "No FLOWRATE_MON_FILE found"
    
    sed -i "s|^FLOWRATE_MON_FILE .*$|FLOWRATE_MON_FILE $OUTPUT_ABS/2-motivation/$(basename $(grep "^FLOWRATE_MON_FILE" "$configFile" 2>/dev/null | awk '{print $2}'))|g" "$configFile"
    
    echo "Updated FLOWRATE_MON_FILE line:"
    grep "^FLOWRATE_MON_FILE" "$configFile" || echo "No FLOWRATE_MON_FILE found"
    echo ""
done

#######################################
# Run experiments
#######################################
for cfg in "${CONFIG_FILES[@]}"; do
    configFile="$EXAMPLE_DIR/$cfg"
    outputFile="$OUTPUT_DIR/${cfg%.txt}_cbfc_output.txt"
    echo "===================================="
    echo "Running with config: $cfg"
    echo "Output -> $outputFile"
    echo "===================================="

    ./waf --run "2-motivation --conf=$configFile" > "$outputFile"
done

echo "##################################"
echo "#      ALL EXPERIMENTS FINISHED   #"
echo "##################################"