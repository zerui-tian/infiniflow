source config.sh
configFile=$NS3/examples/valve/${1}/${1}_config.txt
cd $NS3

./waf

# ./waf --run "$1 --conf=$configFile"
./waf --run "$1 --conf=$configFile" --command-template="%s --scheduler-type=heap" > "$NS3/examples/valve/${1}/${1}_output.txt"
# gdb --args build/examples/valve/ns3.39-$1-optimized --conf=$configFile

echo "##################################"
echo "#      EXPERIMENTS FINISHED      #"
echo "##################################"

# ./examples/valve/plot/plot.sh

# echo "##################################"
# echo "#       PLOTTING FINISHED        #"
# echo "##################################"
