#Kernel Module 編譯缺 .h → 檢查 Kernel headers → 重新安裝對應 headers 套件 → 再 make

#Kbuild
obj-m += chardev_abc.o
obj-m += slave.o
obj-m += request.o
chardev_abc-objs := chardev.o


cc_flags-y += -DFLAG

all:	
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean