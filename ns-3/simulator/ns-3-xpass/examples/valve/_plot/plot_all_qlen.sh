algNames=("dcqcn" "powerInt" "hpcc" "powerDelay" "timely" "dctcp")

for alg in "${algNames[@]}"; do
    echo "Plotting queue length for $alg"
    python3 plot_qlen.py simple-incast $alg png
done