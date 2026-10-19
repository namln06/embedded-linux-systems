# LAB02 - Embedded Linux Systems

## Overview

This repository contains the implementation and test files for Embedded Linux Lab 02.

The lab covers Linux character device drivers, kernel interfaces, NAND flash simulation, and booting a custom Linux system with QEMU.

## Environment

- Linux kernel: 5.15.223
- Architecture: ARM 32-bit (ARM EABI5)
- Emulation platform: QEMU `vexpress-a9`
- Root filesystem: BusyBox-based initramfs

## Lab Contents

### Part A - Character Device Driver

- `driver/lab2_driver.c`: character device driver source code.
- `driver/Makefile`: external kernel module build rules.
- `driver/lab2_driver.ko`: compiled ARM kernel module.

The driver provides device access through `/dev/lab2`.

### Part B - procfs and sysfs

The driver exposes runtime information through:

- `/proc/lab2_info`
- `/sys/class/lab2_class/lab2/buffer_len`
- `/sys/class/lab2_class/lab2/open_count`
- `/sys/class/lab2_class/lab2/last_data`

### Part C - NAND Flash Simulation

The `mtd/dumps/lab2_nand_dump.bin` file contains a NAND dump produced during the lab.

The NAND simulation exercises flash operations and filesystem persistence using the Linux MTD subsystem.

### Part D - QEMU Boot and Driver Test

The `rootfs/initramfs/` directory contains the initramfs startup configuration and driver test script.

Important files:

- `rootfs/initramfs/init`
- `rootfs/initramfs/etc/inittab`
- `rootfs/initramfs/etc/init.d/rcS`
- `rootfs/initramfs/usr/bin/test_driver.sh`
- `rootfs/initramfs/lib/modules/5.15.223/extra/lab2_driver.ko`

The test script checks driver loading, device-node availability, read/write operations, and procfs/sysfs information.

## Repository Structure

```text
LAB02_EmbeddedLinux/
├── README.md
├── docs/
│   └── LAB02_EmbeddedLinux.docx
├── configs/
│   └── kernel-5.15.223.config
├── driver/
│   ├── Makefile
│   ├── lab2_driver.c
│   └── lab2_driver.ko
├── mtd/
│   └── dumps/
│       └── lab2_nand_dump.bin
└── rootfs/
    └── initramfs/
        ├── init
        ├── etc/
        │   ├── inittab
        │   └── init.d/rcS
        ├── lib/modules/5.15.223/extra/lab2_driver.ko
        └── usr/bin/test_driver.sh
```

## Build and Test Notes

The kernel module must be built against the matching Linux kernel build tree and configuration.

The supplied `.ko` files are compiled for ARM and are not directly loadable into a typical x86-64 host kernel.

The initramfs startup scripts and test script are included for inspection and use in the lab environment.

## Documentation

See `docs/LAB02_EmbeddedLinux.docx` for the lab report.
