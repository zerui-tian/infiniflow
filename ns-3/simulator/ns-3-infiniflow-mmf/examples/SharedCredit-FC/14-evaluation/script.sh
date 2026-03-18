SCRIPT_PATH="$(readlink -f "$0")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

CONFIG_FILE="$SCRIPT_DIR/../config.sh"
source $CONFIG_FILE

configFile=$EXAMPLE_DIR/14-evaluation/14-evaluation_config.txt
outputFile=$OUTPUT/14-evaluation/14-evaluation.out
FLOWTHROUGHPUT_PATH="$OUTPUT/14-evaluation/flowthroughput.txt"

mkdir -p $OUTPUT/14-evaluation

sed -i "s|^FLOWTHROUGHPUT_MON_FILE .*|FLOWTHROUGHPUT_MON_FILE $FLOWTHROUGHPUT_PATH|" "$configFile"

cd $NS3

./waf
./waf --run "14-evaluation --conf=$configFile" > $outputFile
# gdb --args build/examples/SharedCredit-FC/ns3.39-$1-optimized --conf=$configFile

echo "##################################"
echo "#      EXPERIMENTS FINISHED      #"
echo "##################################"