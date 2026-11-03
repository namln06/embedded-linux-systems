KDIR := $(HOME)/embedded_lab1/kernel/linux-5.15.223
ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-

obj-m += sms_sensor_driver.o

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
