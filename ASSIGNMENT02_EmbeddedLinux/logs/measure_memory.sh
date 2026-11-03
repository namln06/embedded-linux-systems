#!/bin/bash

cd ~/embedded_assignment2/app || exit 1

rm -f ../logs/memory_report.txt
rm -f ../logs/sms_process_maps.txt

./sms_app_x86 -d 60 -v > ../logs/sms_memory_run.txt 2>&1 &
PID=$!

echo "SMS_APP PID=$PID"

for T in 5 30 55
do
    sleep $(( T - PREVIOUS ))
    echo "===== T=$T seconds =====" | tee -a ../logs/memory_report.txt
    ps -p "$PID" -o pid,stat,rss,vsz,comm | tee -a ../logs/memory_report.txt
    PREVIOUS=$T
done

cat "/proc/$PID/maps" > ../logs/sms_process_maps.txt

wait "$PID"
echo "Memory measurement completed."
