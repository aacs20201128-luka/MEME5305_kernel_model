//modinfo xxx.ko
//ls -la /sys/module/
//vim /usr/include/linux/stat.h
//vim usr/src/linux-headers-6.18.34+rpt-common-rpi/include/linux/kern_levels.h
#include <linux/kernel.h>//為了使用 printk 等核心日誌函式。
#include <linux/module.h>//開發 Kernel Module 必備。
#include <linux/init.h>//為了使用 __init 和 __exit 巨集。
#include <linux/version.h>//if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,10))
#include <linux/kmod.h>//除(insmod/modprobe) 外，從驅動程式內部的程式碼中，主動去呼叫載入其他的模組
#include <linux/fs.h>//註冊 API (新版) dev_t、register_chrdev_region、alloc_chrdev_region

#include "slave.h"
#include "request.h"

static int myint = 3;
static char *mystring = "abc";
module_param(myint, int , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
module_param(mystring, charp, 0644);
MODULE_PARM_DESC(myint,"parameters1_int");
MODULE_PARM_DESC(mystring,"parameters2_charp");

static dev_t devno=0;
static struct cdev mycdev;
static int Major=0;
#define DEVICE_NAME "pico_sensor"
#define BUF_LEN 80
static char Message[BUF_LEN], *msg_Ptr;
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);//函式宣告，參數名稱可以省略。
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
}

static int device_open(struct inode *inode, struct file *file){
    static int counter = 0;
    sprintf(Message, "counter=%d", counter++);
    msg_Ptr = Message;
    return 0;
}
static int device_release(struct inode *inode, struct file *file){
    return 0;
}
static ssize_t device_read(struct file *filp, char *buffer, size_t length, loff_t *offset){
    //buffer 和 length 很直接來自 user space；但 filp、offset 是 kernel 幫你轉換/準備好的。
    if (*msg_Ptr == 0)return 0;
    int bytes_read = 0;
    while (length && *msg_Ptr) {
        put_user(*(msg_Ptr++), buffer++);//往下一個 byte 移。
        length--;
        bytes_read++;
    }
    return bytes_read;
}
static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t *off){
    int i;
    for (i=0; i < len; i++) {
        get_user(Message[i], buff + i);
    }
    msg_Ptr = Message;
    return i;
}

static int __init hello_module(void){
    printk(KERN_EMERG "%s\n",mystring);
    slave_test();
    request_module("request");//request.c:14

    if(alloc_chardev_region(&devno, 0 , 1 ,DEVICE_NAME) < 0){//已指定baseminor
        printk ("alloc_chrdev_region() fail\n");
        return -1;
    }
    Major=MAJOR(devno);
    cdev_init(&mycdev , &fops);
    if(cdev_add(&mycdev, devno , 1)){
        printk ("Error adding cdev\n");
    }
    printk("I was assigned major number %d. To talk to\n", Major);

    return 0;
}

static void __exit bye_module(void){
    printk("Bye\n");

    cdev_del(&mycdev);
    unregister_chrdev_region(devno , 1);
    printk("unregister_chrdev\n");
}

module_init(hello_module);
module_exit(bye_module);
MODULE_LICENSE("GPL");//宣告為開源
MODULE_AUTHOR("aacs20201128@gmail.com");
MODULE_DESCRIPTION("custom kernel space module");
// put_user(x, ptr)：適合放「單一簡單值」，例如 char、int。
// copy_to_user(dst, src, n)：適合複製一整塊 buffer / struct。