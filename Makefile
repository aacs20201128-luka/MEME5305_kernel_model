#Kernel Module 編譯缺 .h → 檢查 Kernel headers → 重新安裝對應 headers 套件 → 再 make

#Kbuild
obj-m += chardev_abc.o
obj-m += slave.o
obj-m += request.o
chardev_abc-objs := chardev.o


cc_flags-y += -DFLAG #ccflags-y

all:	
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean


#CROSS_COMPILE=aarch64-linux-gnu-
CC=$(CROSS_COMPILE)gcc
obj-m += kernel_ioctl.o
KDIR=/lib/modules/$(shell uname -r)/build

ioctl:
	make -C $(KDIR) M=$(shell pwd) kernel_ioctl.ko

obj-m += dht11_sensor.o
dht11:
	make -C $(KDIR) M=$(shell pwd) dht11_sensor.ko

obj-m += pico_sensor.o
pico:
	make -C $(KDIR) M=$(shell pwd) pico_sensor.ko