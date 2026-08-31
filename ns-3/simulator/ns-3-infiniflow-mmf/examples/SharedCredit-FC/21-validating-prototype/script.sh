SCRIPT_PATH="$(readlink -f "$0")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

CONFIG_FILE="$SCRIPT_DIR/../config.sh"
source $CONFIG_FILE

configFile=$EXAMPLE_DIR/21-validating-prototype/21-validating-prototype_config.txt
outputFile=$OUTPUT/21-validating-prototype/21-validating-prototype.out
HEADER_FILE="$NS3/src/point-to-point/model/global-config.h"
cd $NS3

sed -i "s/#define OUTPUT_1 [0-9]*/#define OUTPUT_1 1/" "$HEADER_FILE"

./waf
./waf --run "21-validating-prototype --conf=$configFile" > $outputFile
# gdb --args build/examples/SharedCredit-FC/ns3.39-$1-optimized --conf=$configFile

echo "##################################"
echo "#      EXPERIMENTS FINISHED      #"
echo "##################################"
