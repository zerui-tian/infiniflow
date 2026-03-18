source ../config.sh
modelNames=("vgg16" "resnet50")
configFile=$NS3/examples/valve/dml_scenario/dml_scenario_config_temp/config-dml.txt

cd $NS3
./waf

for model in ${modelNames[@]};do
    i=1
    file_count=$(ls -1 /home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/dml_scenario/dml_scenario_flow_config/$model | wc -l)
    mkdir /home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/dml_scenario/monitor_output/$model
    while [ "$i" -le "$file_count" ]
    do
        common_file="/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/dml_scenario/dml_scenario_config_temp/config-dml-common.txt"
        config_file="/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/dml_scenario/dml_scenario_config_temp/config-dml.txt"
        new_content_1="FLOW_FILE /home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/dml_scenario/dml_scenario_flow_config/"$model"/iteration_"$i".txt"
        new_content_2="FCT_OUTPUT_FILE /home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/dml_scenario/monitor_output/$model/fct_"$i".txt"
        new_content_3="PFC_OUTPUT_FILE /home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/dml_scenario/monitor_output/$model/pfc_"$i".txt"
        combined_content="$(cat "$common_file")"$'\n'"$new_content_1"$'\n'"$new_content_2"$'\n'"$new_content_3"
        echo "$combined_content" > "$config_file"

        echo $model": iteration "$i

        ./waf --run "dml_scenario --conf=$config_file"
        # gdb --args build/examples/valve/ns3.39-dml_scenario-optimized --conf=$configFile

        echo "##################################"
        echo "#      EXPERIMENTS FINISHED      #"
        echo "##################################"

        i=$((i+1))
        
    done
done