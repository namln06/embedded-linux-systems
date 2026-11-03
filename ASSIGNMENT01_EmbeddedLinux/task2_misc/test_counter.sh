#!/bin/sh

MODULE="/root/counter_driver.ko"
TEST="/root/test_counter"
DEVICE="/dev/counter"
PROC_INFO="/proc/counter_info"

FAILURES=0

echo "========================================"
echo " Assignment 01 - Task 2 automated test"
echo "========================================"

echo
echo "[1] Loading counter driver..."

if ! insmod "$MODULE"; then
    echo "[FAIL] Cannot load counter_driver.ko"
    exit 1
fi

echo "[PASS] Driver loaded"

if [ -e "$DEVICE" ]; then
    echo "[PASS] /dev/counter exists"
else
    echo "[FAIL] /dev/counter does not exist"
    FAILURES=$((FAILURES + 1))
fi

echo
echo "[2] Running counter test program..."

"$TEST"
TEST_RESULT=$?

if [ "$TEST_RESULT" -eq 0 ]; then
    echo "[PASS] Counter test program"
else
    echo "[FAIL] Counter test program"
    FAILURES=$((FAILURES + 1))
fi

echo
echo "[3] Checking proc information..."

if [ -e "$PROC_INFO" ]; then
    cat "$PROC_INFO"
    echo "[PASS] /proc/counter_info exists"
else
    echo "[FAIL] /proc/counter_info does not exist"
    FAILURES=$((FAILURES + 1))
fi

echo
echo "[4] Unloading counter driver..."

if rmmod counter_driver; then
    echo "[PASS] Driver unloaded"
else
    echo "[FAIL] Cannot unload driver"
    FAILURES=$((FAILURES + 1))
fi

echo
echo "[5] Checking cleanup..."

if [ ! -e "$DEVICE" ]; then
    echo "[PASS] /dev/counter removed"
else
    echo "[FAIL] /dev/counter still exists"
    FAILURES=$((FAILURES + 1))
fi

if [ ! -e "$PROC_INFO" ]; then
    echo "[PASS] /proc/counter_info removed"
else
    echo "[FAIL] /proc/counter_info still exists"
    FAILURES=$((FAILURES + 1))
fi

echo
echo "========================================"

if [ "$FAILURES" -eq 0 ]; then
    echo "FINAL RESULT: PASS (0 failures)"
else
    echo "FINAL RESULT: FAIL ($FAILURES failures)"
fi

echo "========================================"

exit "$FAILURES"
