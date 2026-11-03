
CC = gcc
ARM_CC = arm-linux-gnueabihf-gcc

CFLAGS = -Wall -Wextra -O2 -pthread -D_POSIX_C_SOURCE=200809L
LDFLAGS = -pthread -lrt

SOURCES = sms_app.c sensor_thread.c processor_thread.c \
          alert_thread.c logger_thread.c circular_buffer.c

HOST_TARGET = sms_app_x86
ARM_TARGET = sms_app

.PHONY: all host arm tests clean

all: host

host: $(HOST_TARGET)

arm: $(ARM_TARGET)

$(HOST_TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -DSMS_HOST_TEST $(SOURCES) -o $@ $(LDFLAGS)

$(ARM_TARGET): $(SOURCES)
	$(ARM_CC) $(CFLAGS) -static $(SOURCES) -o $@ $(LDFLAGS)

jitter_test:
	$(CC) -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200809L jitter_test.c -o $@

jitter_test_arm:
	$(ARM_CC) -Wall -Wextra -O2 -static -D_POSIX_C_SOURCE=200809L jitter_test.c -o $@

tests: test_ioctl test_interval

test_ioctl: test_ioctl.c
	$(ARM_CC) -Wall -Wextra -static test_ioctl.c -o test_ioctl

test_interval: test_interval.c
	$(ARM_CC) -Wall -Wextra -static test_interval.c -o test_interval

clean:
	rm -f $(HOST_TARGET) $(ARM_TARGET) \
	      jitter_test jitter_test_arm test_ioctl test_interval

