obj-m += counter_driver.o

KDIR := $(HOME)/embedded_lab1/kernel/linux-5.15.223
ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-

.PHONY: all clean

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
