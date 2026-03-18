SCRIPT_PATH="$(readlink -f "$0")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

CONFIG_FILE="$SCRIPT_DIR/../config.sh"
source $CONFIG_FILE
configFile=$EXAMPLE_DIR/ablation-study/ablation-study_config.txt
outputFile=$EXAMPLE_DIR/ablation-study/monitor_output/ablation-study.out
OUTPUT_ABS="$(readlink -f "$OUTPUT")"

mkdir -p "$(dirname "$outputFile")"

cd $NS3

NEW_PATH="$OUTPUT_ABS/13-evaluation-ablation-study/sendingrate_nothreshold.txt"

sed -i "s|^FLOWRATE_MON_FILE .*$|FLOWRATE_MON_FILE $NEW_PATH|g" "$configFile"

./waf
./waf --run "ablation-study --conf=$configFile" > $outputFile
# gdb --args build/examples/SharedCredit-FC/ns3.39-$1-optimized --conf=$configFile

echo "##################################"
echo "#      EXPERIMENTS FINISHED      #"
echo "##################################"