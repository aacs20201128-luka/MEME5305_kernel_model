#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include "slave.h"

void slave_test(void){
    printk(KERN_EMERG "slave()\n");
}

//而如果兩個 .c 最後是編成「同一個 .ko」，通常不需要 EXPORT_SYMBOL()；
EXPORT_SYMBOL(slave_test);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("slave module");