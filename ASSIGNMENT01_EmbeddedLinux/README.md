# Assignment 01 – How to Write a Device Driver

**Course:** Embedded Linux Systems  
**Student:** Le Nhat Nam  
**Student ID:** SE205267

## 1. Overview

This assignment focuses on Linux device driver development for the ARM-based Embedded Linux environment.

It consists of two tasks:

- **Task 1 – IOCTL Character Device Driver:** Implements a character device driver with read/write operations and four IOCTL commands for buffer management, statistics, operating mode configuration, and version retrieval.
- **Task 2 – Miscellaneous Counter Driver:** Implements a miscellaneous character device using `miscdevice`, `atomic_t`, and procfs to manage and monitor a counter.

Both drivers are developed as loadable kernel modules and tested using QEMU.

## 2. Development Environment

| Component | Configuration |
|---|---|
| Host OS | Ubuntu 22.04.5 LTS |
| Linux Kernel | 5.15.223 |
| Architecture | ARMv7 |
| Cross Compiler | arm-linux-gnueabihf-gcc 11.4 |
| Emulator | QEMU 6.2.0 |
| QEMU Machine | vexpress-a9 |
| Memory | 512 MB |

The drivers are built for the ARM target using the Linux kernel source tree.

## 3. Project Structure

```text
embedded_assignment1/
│
├── task1_ioctl/
│   ├── asgn1_driver.c
│   ├── asgn1_ioctl.h
│   ├── asgn1_driver.ko
│   ├── asgn1_test.c
│   ├── asgn1_test
│   ├── test_asgn1.sh
│   └── Makefile
│
├── task2_misc/
│   ├── counter_driver.c
│   ├── counter_driver.ko
│   ├── test_counter.c
│   ├── test_counter
│   ├── test_counter.sh
│   └── Makefile
│
├── logs/
│   ├── test_asgn1_output.txt
│   └── test_counter_output.txt
│
├── initramfs_asgn1.cpio.gz
└── README.md
```

## 4. Task 1 – IOCTL Character Device Driver

### 4.1. Description

Task 1 implements a character device named `/dev/asgn1` with the following features:

- Character device registration using a fixed major number.
- Read and write operations.
- Internal buffer management with a mutex.
- Open, read, and write statistics.
- Two operating modes:
  - **Kernel Mode (0):** Adds the `[KERNEL]` prefix to written data.
  - **Echo Mode (1):** Stores the original written data.
- Four IOCTL commands.

### 4.2. IOCTL Commands

| Command | Description | Direction |
|---|---|---|
| `ASGN1_RESET_BUFFER` | Clears the buffer and resets read/write statistics | `_IO` |
| `ASGN1_GET_STATS` | Retrieves driver statistics | `_IOR` |
| `ASGN1_SET_MODE` | Configures the driver operating mode | `_IOW` |
| `ASGN1_GET_VERSION` | Retrieves the driver version | `_IOR` |

The driver uses `copy_from_user()` and `copy_to_user()` for data transfer between user space and kernel space.

### 4.3. Testing

The test program `asgn1_test` checks:

- Buffer reset.
- Driver version retrieval.
- Echo mode read/write.
- Kernel mode read/write.
- Statistics retrieval.
- Invalid mode handling.
- Unknown IOCTL handling.
- Final buffer reset.

Test script: `test_asgn1.sh`

## 5. Task 2 – Miscellaneous Counter Driver

### 5.1. Description

Task 2 implements a miscellaneous device named `/dev/counter`.

The driver uses:

- `struct miscdevice` for device registration.
- `atomic_t` for counter and statistics management.
- `misc_register()` and `misc_deregister()` for registration and cleanup.
- `kstrtoint()` for parsing user input.
- `procfs` to expose driver information.

### 5.2. Device Operations

| Operation | Description |
|---|---|
| Read | Returns the current counter value and increments it |
| Write | Sets the counter to a user-provided non-negative integer |
| Invalid input | Returns `-EINVAL` |
| EOF | Returns 0 when the file position has reached the end of the current read result |

### 5.3. Procfs Interface

The driver creates:

```text
/proc/counter_info
```

It reports:

| Field | Description |
|---|---|
| `current_counter` | Current counter value |
| `total_reads` | Total successful read operations |
| `total_increments` | Total counter increments |
| `total_writes` | Total successful write operations |
| `total_resets` | Number of writes that set the counter to zero |

### 5.4. Testing

The test program `test_counter` checks:

- Initial counter value.
- Automatic increment after reading.
- Writing a new counter value.
- Reading the written value.
- Rejection of negative values.
- Rejection of non-numeric input.
- Resetting the counter.
- EOF behavior.
- Procfs statistics.

Test script: `test_counter.sh`

## 6. Building the Drivers

The drivers can be compiled on the Ubuntu host using the provided Makefiles.

Example for Task 1:

```bash
cd task1_ioctl

make
```

Example for Task 2:

```bash
cd task2_misc

make
```

The generated kernel modules are:

- `asgn1_driver.ko`
- `counter_driver.ko`

The modules must be built for the same kernel version and architecture as the QEMU target.

## 7. Running on QEMU

The assignment uses the ARM `vexpress-a9` machine.

Launch QEMU from the Ubuntu host:

```bash
qemu-system-arm \
    -M vexpress-a9 \
    -m 512M \
    -kernel ~/embedded_lab1/output/zImage \
    -dtb ~/embedded_lab1/output/vexpress-v2p-ca9.dtb \
    -initrd ~/embedded_assignment1/initramfs_asgn1.cpio.gz \
    -append "console=ttyAMA0,115200 rdinit=/init" \
    -nographic
```

After the Linux system boots, the test programs and scripts can be executed inside the QEMU environment.

### Task 1

```bash
/root/test_asgn1.sh
```

### Task 2

```bash
/root/test_counter.sh
```

## 8. Test Results

Both drivers have been tested in the QEMU ARM environment.

| Test | Result |
|---|---|
| Task 1 – IOCTL Character Device | PASS |
| Task 2 – Miscellaneous Counter Device | PASS |
| Task 2 – Invalid input handling | PASS |
| Task 2 – Procfs statistics | PASS |
| Task 2 – Module unload | PASS |
| Task 2 – Device and procfs cleanup | PASS |

The Task 2 automated test script completed with:

```text
FINAL RESULT: PASS (0 failures)
```

Detailed test outputs are available in the `logs/` directory.

## 9. Notes

- The `.ko` files are included as prebuilt ARM kernel modules.
- The test executables are compiled for the ARM target.
- The provided QEMU environment uses Linux kernel version `5.15.223`.
- Modules must match the target kernel version and configuration.
- The initramfs archive provides the test environment used for this assignment.

---

**Assignment 01 – Embedded Linux Systems**