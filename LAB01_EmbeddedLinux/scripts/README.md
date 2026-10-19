# Scripts and Test Instructions

## Current Status

This directory documents the test procedure for LAB 01.
No automated build or test scripts are included yet.

## Boot the Kernel with QEMU

Run this command from the `LAB01_EmbeddedLinux` directory:

    qemu-system-arm \
      -M vexpress-a9 \
      -m 512M \
      -kernel output/zImage \
      -dtb output/vexpress-v2p-ca9.dtb \
      -initrd output/initramfs.cpio.gz \
      -append "console=ttyAMA0,115200 rdinit=/init" \
      -nographic

## Verify the Booted System

At the BusyBox shell, run:

    uname -a
    ls /
    busybox

The direct kernel boot test completed successfully.
U-Boot has been built, but booting through U-Boot has not yet been verified.
