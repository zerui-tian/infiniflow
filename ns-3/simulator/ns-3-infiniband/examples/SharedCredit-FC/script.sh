source config.sh
configFile=$NS3/examples/SharedCredit-FC/${1}/${1}_config.txt
outputFile=$NS3/examples/SharedCredit-FC/${1}/${1}_output.txt
cd $NS3

./waf
./waf --run "$1 --conf=$configFile" > $outputFile
# gdb --args build/examples/valve/ns3.39-$1-optimized --conf=$configFile

echo "##################################"
echo "#      EXPERIMENTS FINISHED      #"
echo "##################################"

# ./examples/valve/plot/plot.sh

# echo "##################################"
# echo "#       PLOTTING FINISHED        #"
# echo "##################################"