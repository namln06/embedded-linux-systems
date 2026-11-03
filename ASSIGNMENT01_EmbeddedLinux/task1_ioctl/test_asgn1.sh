#!/bin/sh

DEVICE="/dev/asgn1"
TEST="./asgn1_test"

echo "========================================"
echo " Assignment 01 - Task 1 ioctl test"
echo "========================================"

if [ ! -f "$TEST" ]; then
    echo "[FAIL] Missing test program: $TEST"
    exit 1
fi

if [ ! -e "$DEVICE" ]; then
    echo "[FAIL] Device $DEVICE does not exist."
    echo "[INFO] Load asgn1_driver.ko inside QEMU first."
    exit 1
fi

echo "[INFO] Running ioctl tests..."
"$TEST"
RESULT=$?

echo
if [ "$RESULT" -eq 0 ]; then
    echo "===== Result: PASS ====="
else
    echo "===== Result: FAIL (exit code $RESULT) ====="
fi

exit "$RESULT"
