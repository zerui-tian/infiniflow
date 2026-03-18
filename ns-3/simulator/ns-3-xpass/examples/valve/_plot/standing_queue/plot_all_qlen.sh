algNames=("dcqcn" "powerInt" "hpcc" "timely")

for alg in "${algNames[@]}"; do
    echo "Plotting queue length for $alg"
    python3 plot_qlen.py standing_queue $alg png
done