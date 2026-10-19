#!/bin/sh

echo "===== LAB-02 DRIVER TEST ====="

echo
echo "[1] Module status"
cat /proc/modules | grep lab2_driver || echo "Module not listed"

echo
echo "[2] Character device"
ls -l /dev/lab2

echo
echo "[3] Write and read"
echo "Hello from LAB-02 test script" > /dev/lab2
cat /dev/lab2

echo
echo "[4] procfs"
cat /proc/lab2_info

echo
echo "[5] sysfs"
cat /sys/class/lab2_class/lab2/buffer_len
cat /sys/class/lab2_class/lab2/open_count
cat /sys/class/lab2_class/lab2/last_data

echo
echo "===== TEST SCRIPT FINISHED ====="
