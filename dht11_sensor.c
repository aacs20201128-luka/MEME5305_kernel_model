#include <linux/kernel.h>//為了使用 printk 等核心日誌函式。
#include <linux/module.h>//開發 Kernel Module 必備。
#include <linux/init.h>//為了使用 __init 和 __exit 巨集。
#include <linux/version.h>//if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,10))
#include <linux/kmod.h>//除(insmod/modprobe) 外，從驅動程式內部的程式碼中，主動去呼叫載入其他的模組
#include <linux/fs.h>//註冊 API (新版) dev_t、register_chrdev_region、alloc_chrdev_region
#include <linux/cdev.h>//cdev_init()初始化struct cdev，之後再交給cdev_add()加進系統。
#include <linux/uaccess.h>// put_user(), get_user(), copy_to_user(), copy_from_user()
#include <linux/string.h>// strlen(), strcpy(), strcmp(), memset()...
#include <linux/errno.h>// Linux 核心錯誤碼定義，如 EINVAL、ENOMEM、EIO
#include <linux/device.h>//告別手動 mknod
#include <linux/signal.h>//發送 Signal 給 User Space
#include <linux/sched.h>//Wait Queue
#include <linux/io.h>//for ioremap(), iounmap(), readl(), writel()
#include <linux/delay.h> //可使用 msleep()、mdelay()、udelay() 等延遲函式
#include <linux/major.h>   // Linux 已知／保留的 Major Number 定義
#include <linux/poll.h>    // poll/select/epoll 相關機制
#include <linux/mutex.h>   // Mutex 互斥鎖

#include "./dht11_sensor.h"

static dev_t devno=0;
static struct cdev mycdev;
static int Major=0;
static char Message[4] , *msg_Ptr;
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
};
static struct class *charmodule_dht11_class;

static inline int dht11_read_pin(void){
    // 現在 HIGH → return 1
    // 現在 LOW  → return 0
    u32 value;
    value = readl((u8 __iomem *)RIOBase + RIO_IN_OFFSET);
    return (value & (1 << pin)) ? 1 : 0;
}
static inline int dht11_get_data(int *timeout, int *bit){
    *timeout = 100;
    while (dht11_read_pin() == 0) {//只有在高電位才會用us判斷0 or 1
        if (--(*timeout) == 0) {
            printk("DHT11: data LOW timeout\n");
            return -ETIMEDOUT;
        }
        udelay(1);
    }
    // LOW ~50us   HIGH ~26-28us   -> 0
    // LOW ~50us   HIGH ~70us      -> 1
    udelay(40);//選一個介於兩者中間的時間
    //如果40us後if (dht11_read_pin() == 1)是低電壓就else如果還是高電壓就代表是1
    if (dht11_read_pin() == 1) {
        *bit = 1;   /* HIGH 還在，表示約 70us */     
        *timeout = 100;
        while (dht11_read_pin() == 1) {/* 等目前這個 bit 的 HIGH 結束 */
            if (--(*timeout) == 0){
                printk("DHT11: data HIGH timeout\n");
                return -ETIMEDOUT;
            }
            udelay(1);
        }
    }
    else *bit = 0;                         /* 已經 LOW，表示只有約 26~28us */
    return 0;
}

static int device_open(struct inode *inode, struct file *file){
    writel( fn , (u8 __iomem *)GPIOBase + GPIO_CTRL_OFFSET +  GPIO_STRIDE * pin);
  	writel( 0x58 , (u8 __iomem *)pad + PAD_STRIDE * pin);//開IE、4mA、開Pull up 
    // writel( 1 << pin , (u8 __iomem *) RIOBase + RIO_SET_OFFSET + RIO_OE_OFFSET);
    // writel( 1 << pin , (u8 __iomem *) RIOBase + RIO_SET_OFFSET + RIO_OUT_OFFSET);
    // msleep(1000);
    return 0;
}
static int device_release(struct inode *inode, struct file *file){
    // writel( 1 << pin , (u8 __iomem *) RIOBase + RIO_CLR_OFFSET + RIO_OUT_OFFSET);
    // writel( 1 << pin , (u8 __iomem *) RIOBase + RIO_CLR_OFFSET + RIO_OE_OFFSET);
	  return 0;
}
static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset){
    if (*offset > 0)//已經讀過這筆資料
        return 0;
    
    //輸出時，我控制電位和時間；輸入時，我主要等對方的邊沿變化，必要時再用 delay 做取樣判斷。
    //MCU Sends out Start Signal to DHT
    writel( 1 << pin , (u8 __iomem *) RIOBase + RIO_SET_OFFSET + RIO_OE_OFFSET);//啟用輸出
    writel( 1 << pin , (u8 __iomem *) RIOBase + RIO_CLR_OFFSET + RIO_OUT_OFFSET);//輸出低
    mdelay(18);
    writel( 1 << pin , (u8 __iomem *) RIOBase + RIO_SET_OFFSET + RIO_OUT_OFFSET);//輸出高
    udelay(20);
    writel( 1 << pin , (u8 __iomem *) RIOBase + RIO_CLR_OFFSET + RIO_OE_OFFSET);//切回輸入

    //DHT Sends out response signal
    int timeout = 100;
    while (dht11_read_pin() == 1) {//等拉低
        if (--timeout == 0) {
            printk("DHT11: no response\n");
            return -ETIMEDOUT;
        }
        udelay(1);
    }
    //Once DHT detects the start signal, it will send out a low-voltage-level response 
    // signal, which lasts 80us. 
    timeout = 100;
    while (dht11_read_pin() == 0) {//等拉高
        if (--timeout == 0) {
            printk("DHT11: response LOW timeout\n");
            return -ETIMEDOUT;
        }
        udelay(1);
    } 
    //Once DHT sent out the response signal, it pulls up voltage and keeps it for 80us and 
    // prepares for data transmission. 
    timeout = 100;
    while (dht11_read_pin() == 1) {
        if (--timeout == 0) {
            printk("DHT11: response HIGH timeout\n");
            return -ETIMEDOUT;
        }
        udelay(1);
    }
    //Start data transmission
    //high-voltage-level signal determines whether data bit is "0" or "1" 
    int bit = 0;
    int bit_fer[40]; 
    int ret;
    u8 data[5] = {0};//全清0 每個元素8bit...
    for(int i = 0; i < 40; i++){
        ret = dht11_get_data(&timeout , &bit);
        if (ret < 0) {
            printk("DHT11: read bit %d failed\n", i);
            return ret;
        }
        bit_fer[i] = bit;
    }
    // 40 bit -> 5 bytes
    for (int i = 0; i < 40; i++) {
        data[i / 8] <<= 1;  //取出目前要寫入的 byte，所有 bit 左移一格，最低位補 0
        data[i / 8] |= bit_fer[i]; //將目前讀到的 0/1 寫入 byte 的最低位
    }
    // checksum 「+」是「數值相加」，不是「bit 串接」。
    // check-sum should be the last 8bit 
    if (((data[0] + data[1] + data[2] + data[3]) & 0xff) != data[4]) {
        printk("DHT11: checksum error\n");
        return -EIO;
    }
    //8bit integral RH data + 8bit decimal RH data 
    //8bit integral T data + 8bit decimal T data
    printk("Humidity: %u.%u %%\n", data[0], data[1]);
    printk("Temperature: %u.%u C\n", data[2], data[3]);

    /* send 4 bytes to userspace */
    size_t len;
    len = min(length, (size_t)4);
    if (copy_to_user(buffer, data, len))
        return -EFAULT;
    *offset += len;
    return len;
}
static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t *off){
    return 0;
}

static __init int dht11_init(void){
    if(alloc_chrdev_region(&devno, 0 , 1 ,DEVICE_NAME) < 0){//devno寫入新的值。
      printk ("alloc_chrdev_region() fail\n");
      return -1;
    }
    Major=MAJOR(devno);
    cdev_init(&mycdev , &fops);
    if(cdev_add(&mycdev, devno , 1)){
      printk ("Error adding cdev\n");
    }
    printk("I was assigned major number %d.\n", Major);

    charmodule_dht11_class = class_create("charmodule_dht11_class");//設備類別
    device_create(charmodule_dht11_class, NULL, devno, NULL, DEVICE_NAME);//設備節點

    PERIBase = ioremap(0x1f00000000, 64 * 1024 * 1024);
    if(PERIBase == NULL)printk("ioremap() fail\n");
    GPIOBase = (u8 __iomem *)PERIBase + 0xD0000;
    RIOBase = (u8 __iomem *)PERIBase + 0xE0000;	
    PADBase = (u8 __iomem *)PERIBase + 0xF0000;
    pad = (u8 __iomem *)PADBase + 0x04;
    printk("PERIBase:%px\n", PERIBase);	
    printk("GPIOBase:%px\n", GPIOBase);
    printk("RIOBase:%px\n", RIOBase);
    printk("PADBase:%px\n", PADBase);
    printk("pad:%px\n", pad);
    return 0;
}
static __exit void dht11_exit(void){
    cdev_del(&mycdev);

    device_destroy(charmodule_dht11_class, devno);
    class_destroy(charmodule_dht11_class);

    unregister_chrdev_region(devno , 1);
    printk("unregister_chrdev\n");

	iounmap(PERIBase);
}


module_init(dht11_init);
module_exit(dht11_exit);
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("aacs20201128@gmail.com");
MODULE_DESCRIPTION("dht11 sensor driver");

//標準 C 並不允許對 void * 做 pointer arithmetic(指標運算)。
//(u8 __iomem *) 可以明確表示後面的 offset 是以 byte 為單位計算，
//  避免 void * pointer arithmetic 帶來可讀性或編譯器相依性的問題。
// 可讀性會更好：
// void __iomem *reg;
// reg = (u8 __iomem *)GPIOBase
//       + GPIO_CTRL_OFFSET
//       + GPIO_STRIDE * pin;
// writel(fn, reg);


//inline 是在告訴編譯器：這個函式很小，可以考慮把函式內容直接展開到呼叫的位置，減少函式呼叫成本。


//RIO 決定 GPIO 做什麼，PAD 決定這根實體腳怎麼做。
//因此 RIO_IN 是「讀值的 register」；PAD IE 是「讓這個值能不能從實體 pin 進來」。
// RIO：管「邏輯」——這顆 GPIO 現在輸出 0/1、要不要輸出、目前讀到 0/1。
// PAD：管「實體電氣特性」——這根腳實際怎麼跟外部電路互動。
// PAD IE = 允許讀 input
// RIO OE = 決定現在是不是 output
//  OE=0 是把輸出能力關掉。因為不再往外驅動，所以腳位就可以安全地拿來讀外部訊號(所以通常當作輸入)。
// RIO OUT = 決定 output 是 HIGH 還是 LOW

// struct file *filp ;  代表目前被開啟的裝置檔案物件
// char __user *buffer ;  指向 User Space 接收資料的緩衝區
// size_t length ;  User Space 最多想讀取多少 Bytes
// loff_t *offset ; 目前讀取到的位置／檔案偏移量

//read()回傳 0 的意義就是：EOF / 目前沒有更多資料可讀

//min(a, b) // 比較 a 和 b，回傳較小值；回傳型別通常與比較值型別一致