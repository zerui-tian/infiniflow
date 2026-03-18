SCRIPT_PATH="$(readlink -f "$0")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

CONFIG_FILE="$SCRIPT_DIR/../config.sh"
source $CONFIG_FILE
configFile=$NS3/examples/SharedCredit-FC/18-dt-limitation/18-dt-limitation_config.txt
outputFile=$SIMULATION_ROOT/ns-3/simulator/result/data/18-dt-limitation/18-dt-limitation.out

OUTPUT_ABS="$(readlink -f "$OUTPUT")"

mkdir -p "$(dirname "$outputFile")"

cd $NS3

NEW_PATH="$OUTPUT_ABS/18-dt-limitation/sendingrate.txt"

sed -i "s|^FLOWRATE_MON_FILE .*$|FLOWRATE_MON_FILE $NEW_PATH|g" "$configFile"

./waf
./waf --run "18-dt-limitation --conf=$configFile" > $outputFile
# gdb --args build/examples/valve/ns3.39-$1-optimized --conf=$configFile

echo "##################################"
echo "#      EXPERIMENTS FINISHED      #"
echo "##################################"