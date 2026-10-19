# LAB 01 — Embedded Linux Systems

## 1. Overview

This lab builds a minimal Embedded Linux environment for the ARM Versatile Express A9 platform.

The project includes:
- Linux Kernel 5.15.223
- U-Boot 2022.04
- BusyBox 1.35.0
- ARM initramfs root filesystem
- QEMU-based kernel boot testing

## 2. Project Structure

```text
LAB01_EmbeddedLinux/
├── README.md
├── .gitignore
├── configs/
│   ├── kernel.config
│   └── busybox.config
├── output/
│   ├── zImage
│   ├── vexpress-v2p-ca9.dtb
│   ├── initramfs.cpio.gz
│   └── u-boot.bin
├── rootfs/
│   └── initramfs/
│       ├── init
│       ├── linuxrc
│       ├── bin/
│       ├── dev/
│       ├── etc/
│       ├── proc/
│       ├── root/
│       ├── sbin/
│       ├── sys/
│       ├── tmp/
│       └── usr/
└── scripts/
    └── README.md
```

## 3. Build Configuration

| Component | Version / Configuration |
|---|---|
| Linux Kernel | 5.15.223 |
| U-Boot | 2022.04 |
| BusyBox | 1.35.0, static build |
| Target architecture | ARM 32-bit |
| QEMU machine | `vexpress-a9` |
| Memory used in QEMU | 512 MiB |

Configuration files are stored in `configs/`.

## 4. Output Artifacts

| File | Description |
|---|---|
| `output/zImage` | Linux kernel image |
| `output/vexpress-v2p-ca9.dtb` | Device Tree Blob |
| `output/initramfs.cpio.gz` | Compressed initial root filesystem |
| `output/u-boot.bin` | Built U-Boot binary |

## 5. Root Filesystem

The root filesystem is based on BusyBox. The `init` script mounts `/dev`, `/proc`, and `/sys`, prints a startup message, and starts a shell.

The root filesystem is stored in `rootfs/initramfs/` and packaged as `output/initramfs.cpio.gz`.

## 6. QEMU Kernel Boot Test

The kernel and initramfs were tested by booting the kernel directly in QEMU:

```bash
qemu-system-arm \
  -M vexpress-a9 \
  -m 512M \
  -kernel output/zImage \
  -dtb output/vexpress-v2p-ca9.dtb \
  -initrd output/initramfs.cpio.gz \
  -append "console=ttyAMA0,115200 rdinit=/init" \
  -nographic
```

### Expected Result

The system should display the Embedded Linux LAB 1 startup message and provide a BusyBox shell.

Commands used for basic verification include:

```sh
uname -a
ls /
busybox
```

The direct kernel boot test completed successfully. U-Boot was built, but booting through U-Boot has not yet been verified.

## 7. Notes

- The `output/` directory contains the generated artifacts required for inspection and testing.
- Kernel and BusyBox configuration snapshots are included in `configs/`.
- Full kernel and U-Boot source trees are not included in this submission directory.
