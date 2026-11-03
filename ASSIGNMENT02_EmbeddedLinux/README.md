# Assignment 02 – Embedded Linux Sensor Monitoring System

## 1. Overview

This project implements an embedded temperature and humidity monitoring system on Embedded Linux using a custom Linux kernel character device driver and a multithreaded userspace application.

The system runs on an ARMv7-based QEMU virtual platform and demonstrates Linux device driver development, POSIX multithreading, inter-process communication, synchronization, memory management, and real-time scheduling.

## 2. System Architecture

The system consists of two main components:

- **Kernel Space:** A custom SMS sensor driver that exposes temperature and humidity readings through a character device.
- **User Space:** A multithreaded application that collects, processes, monitors, and logs sensor data.

### Application Threads

| Thread | Responsibility |
|---|---|
| Sensor Thread | Reads sensor data from `/dev/sms_sensor` |
| Processor Thread | Calculates rolling averages and detects abnormal readings |
| Alert Thread | Receives and displays alert messages |
| Logger Thread | Records sensor data and manages log rotation |

### Communication and Synchronization

- Character device interface for kernel-to-userspace data transfer.
- POSIX message queues for alert and logging communication.
- Circular buffer for sensor data storage.
- Mutexes and condition variables for thread synchronization.
- `ioctl()` interface for configuring the sensor sampling interval.

## 3. Project Structure

```text
embedded_assignment2/
├── app/
│   ├── sms_app.c
│   ├── sensor_thread.c
│   ├── sensor_thread.h
│   ├── processor_thread.c
│   ├── processor_thread.h
│   ├── alert_thread.c
│   ├── alert_thread.h
│   ├── logger_thread.c
│   ├── logger_thread.h
│   ├── circular_buffer.c
│   ├── circular_buffer.h
│   ├── sms_messages.h
│   ├── jitter_test.c
│   ├── test_ioctl.c
│   ├── test_interval.c
│   └── Makefile
│
├── driver/
│   ├── sms_sensor_driver.c
│   ├── sms_sensor.h
│   ├── sms_sensor_driver.ko
│   └── Makefile
│
├── rootfs/
│   ├── init
│   ├── lib/modules/5.15.223/extra/
│   └── usr/bin/
│
├── output/
│   └── initramfs_asgn2.cpio.gz
│
└── logs/
    ├── driver_test_report.txt
    ├── sms_sample_run.txt
    ├── sms.log
    ├── valgrind_sms.txt
    ├── strace_sms.txt
    ├── perf_sms.txt
    ├── memory_report.txt
    ├── jitter_report.txt
    └── ...
```

## 4. Hardware and Software Environment

| Component | Configuration |
|---|---|
| Host OS | Ubuntu 22.04 LTS |
| Target Architecture | ARMv7 |
| Virtual Platform | QEMU `vexpress-a9` |
| Linux Kernel | 5.15.223 |
| Cross Compiler | `arm-linux-gnueabihf-gcc` |
| Userspace Language | C |
| Concurrency | POSIX Threads |
| IPC | POSIX Message Queues |

## 5. Kernel Driver

The `sms_sensor_driver` module implements a character device named:

```text
/dev/sms_sensor
```

### Features

- Generates simulated temperature readings between 15°C and 45°C.
- Generates simulated humidity readings between 30% and 90%.
- Provides timestamped CSV sensor data.
- Supports configurable sampling intervals through `ioctl()`.
- Exposes driver statistics through `/proc/sms_stats`.

### Sensor Data Format

```text
timestamp_ms,temperature,humidity
```

### IOCTL Interface

The driver supports:

| Command | Description |
|---|---|
| `SMS_SET_INTERVAL` | Configure the sensor sampling interval |

Valid sampling intervals range from 1 ms to 60000 ms.

### Procfs Statistics

The `/proc/sms_stats` interface provides:

- `read_count`
- `last_temperature`
- `last_humidity`
- `uptime`
- `interval_ms`

## 6. Userspace Application

The `sms_app` application implements a four-thread sensor monitoring pipeline.

### Main Features

- Configurable sensor sampling interval.
- Rolling average calculation using the latest 10 readings.
- Temperature and humidity threshold monitoring.
- Temperature spike detection.
- Asynchronous alert delivery through POSIX message queues.
- Sensor data logging to `/tmp/sms.log`.
- Log rotation when the log file exceeds 1 MB.
- Graceful shutdown and resource cleanup.

### Command-Line Options

```text
sms_app [-i ms] [-t temperature] [-h humidity] [-d seconds] [-v]
```

| Option | Description | Default |
|---|---|---|
| `-i` | Sensor sampling interval | 500 ms |
| `-t` | Temperature alert threshold | 40°C |
| `-h` | Humidity alert threshold | 80% |
| `-d` | Application running duration | 60 seconds |
| `-v` | Enable verbose output | Disabled |

Example:

```bash
./sms_app -i 200 -t 40 -h 80 -d 10 -v
```

## 7. Building the Project

### 7.1. Build the Kernel Driver

The driver Makefile expects the Linux kernel source tree at:

```text
~/embedded_lab1/kernel/linux-5.15.223
```

Build the ARM kernel module:

```bash
cd driver
make
```

The generated module is:

```text
sms_sensor_driver.ko
```

### 7.2. Build the Userspace Application

Build the x86 host version:

```bash
cd app
make host
```

Build the ARM target version:

```bash
make arm
```

Build the jitter test programs:

```bash
make jitter_test
make jitter_test_arm
```

Build the driver test programs:

```bash
make tests
```

### 7.3. Build the Initramfs

After copying the latest driver and ARM application into the root filesystem, create the initramfs image:

```bash
cd rootfs

find . -print0 | cpio --null -o --format=newc | gzip -9 \
> ../output/initramfs_asgn2.cpio.gz
```

## 8. Running on QEMU

Launch the ARM virtual platform:

```bash
qemu-system-arm \
  -M vexpress-a9 \
  -m 512M \
  -kernel ~/embedded_lab1/output/zImage \
  -dtb ~/embedded_lab1/output/vexpress-v2p-ca9.dtb \
  -initrd ~/embedded_assignment2/output/initramfs_asgn2.cpio.gz \
  -append "console=ttyAMA0,115200 rdinit=/init" \
  -nographic
```

The initialization script automatically:

1. Mounts `/dev`, `/proc`, and `/sys`.
2. Loads the SMS sensor kernel module.
3. Creates `/dev/sms_sensor`.
4. Starts an interactive shell.

Run the application:

```bash
/usr/bin/sms_app -i 200 -d 10 -v
```

Inspect driver statistics:

```bash
cat /proc/sms_stats
```

Exit QEMU using:

```text
Ctrl + A, then X
```

## 9. Testing and Validation

Testing and analysis results are stored in the `logs/` directory.

The project includes validation for:

- Character device registration and sensor data reading.
- Procfs statistics updates.
- Valid and invalid IOCTL commands.
- Sampling interval validation.
- Multithreaded application execution.
- Circular buffer production and consumption.
- Memory analysis using Valgrind.
- System call analysis using strace.
- Performance profiling using perf.
- Scheduling and timing jitter measurements.

### QEMU Integration Test

The ARM application was tested on QEMU with a 200 ms sampling interval and a 10-second execution duration.

Observed results:

| Metric | Result |
|---|---:|
| Sensor samples produced | 49 |
| Sensor samples consumed | 49 |
| Circular buffer overflow | 0 |
| Temperature spike alerts | 15 |
| Log entries | 49 |
| Driver sampling interval | 200 ms |

The application completed execution and shut down successfully.

## 10. Notes

- Sensor readings are simulated by the kernel driver; no physical sensor hardware is required.
- The ARM application is intended to run inside the QEMU environment.
- The x86 host application is used for host-side testing and analysis.
- Kernel source and QEMU dependencies must be available at the paths specified in the build and run commands.
