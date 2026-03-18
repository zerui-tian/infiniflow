source config.sh
configFile=$NS3/examples/infiniflow/1-control-dynamics/1-control-dynamics_config.txt
outputFile=$NS3/examples/infiniflow/1-control-dynamics/monitor_output/1-control-dynamics.out
cd $NS3

./waf
./waf --run "1-control-dynamics --conf=$configFile" > $outputFile
# gdb --args build/examples/SharedCredit-FC/ns3.39-$1-optimized --conf=$configFile

echo "##################################"
echo "#      EXPERIMENTS FINISHED      #"
echo "##################################"

python3 $EXAMPLE_DIR/1-control-dynamics/_plot/draw_flowthroughput.py $INFINIFLOW_DIR

python3 $EXAMPLE_DIR/1-control-dynamics/_plot/get_inflight_data.py $INFINIFLOW_DIR

python3 $EXAMPLE_DIR/1-control-dynamics/_plot/plot_buffer_size.py $INFINIFLOW_DIR

# echo "##################################"
# echo "#       PLOTTING FINISHED        #"
# echo "##################################"