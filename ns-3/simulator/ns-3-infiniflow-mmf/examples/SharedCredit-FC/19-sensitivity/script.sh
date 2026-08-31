SCRIPT_PATH="$(readlink -f "$0")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

CONFIG_FILE="$SCRIPT_DIR/../config.sh"
source $CONFIG_FILE

configFile=$EXAMPLE_DIR/19-sensitivity/19-sensitivity_config.txt
outputFile=$OUTPUT/19-sensitivity/19-sensitivity.out

mkdir -p $OUTPUT/19-sensitivity

cd $NS3

./waf
./waf --run "19-sensitivity --conf=$configFile" > $outputFile
# gdb --args build/examples/SharedCredit-FC/ns3.39-$1-optimized --conf=$configFile

echo "##################################"
echo "#      EXPERIMENTS FINISHED      #"
echo "##################################"