//modinfo xxx.ko
//ls -la /sys/module/
//vim /usr/include/linux/stat.h
//vim usr/src/linux-headers-6.18.34+rpt-common-rpi/include/linux/kern_levels.h
#include <linux/kernel.h>//為了使用 printk 等核心日誌函式。
#include <linux/module.h>//開發 Kernel Module 必備。
#include <linux/init.h>//為了使用 __init 和 __exit 巨集。
#include <linux/version.h>//if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,10))
#include <linux/kmod.h>//除(insmod/modprobe) 外，從驅動程式內部的程式碼中，主動去呼叫載入其他的模組

#include "slave.h"
#include "request.h"


static int myint = 3;
static char *mystring = "abc";
module_param(myint, int , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
module_param(mystring, charp, 0644);
MODULE_PARM_DESC(myint,"parameters1_int");
MODULE_PARM_DESC(mystring,"parameters2_charp");

static int __init hello_module(void){
    printk(KERN_EMERG "%s\n",mystring);
    slave_test();
    request_module("request");//request.c:14
    return 0;
}

static void __exit bye_module(void){
    printk("Bye\n");
}

module_init(hello_module);
module_exit(bye_module);
MODULE_LICENSE("GPL");//宣告為開源
MODULE_AUTHOR("aacs20201128@gmail.com");
MODULE_DESCRIPTION("custom kernel space module");