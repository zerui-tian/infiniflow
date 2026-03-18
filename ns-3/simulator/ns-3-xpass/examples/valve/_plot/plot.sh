cd /home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/_plot/
python3 ./plot_qlen.py $1 png
echo "Queue length waveform is plotted!"
python3 ./plot_sending_rate.py $1 png
echo "Sending rate waveform is plotted!"
python3 ./plot_throughput.py $1 png
echo "Throughput waveform is plotted!"