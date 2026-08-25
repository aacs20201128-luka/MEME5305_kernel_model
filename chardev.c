//modinfo xxx.ko
//ls -la /sys/module/
//vim /usr/include/linux/stat.h
//vim usr/src/linux-headers-6.18.34+rpt-common-rpi/include/linux/kern_levels.h
//vim /sys/class/charmodule_class
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

#include "slave.h"
#include "request.h"

static int user_pid = 3;
static char *mystring = "abc";
module_param(user_pid, int , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
module_param(mystring, charp, 0644);
MODULE_PARM_DESC(user_pid,"parameters1_int");
MODULE_PARM_DESC(mystring,"parameters2_charp");

static dev_t devno=0;
static struct cdev mycdev;
static int Major=0;
#define DEVICE_NAME "pico_sensor_abc"
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
};
static struct class *charmodule_class;

DECLARE_WAIT_QUEUE_HEAD(wq);
static int bSleep;

volatile uint32_t *PERIBase;//先宣告之後再賦值
volatile uint32_t *GPIOBase;
volatile uint32_t *RIOBase;
volatile uint32_t *PADBase;
volatile uint32_t *pad;
typedef struct{
	uint32_t status;
	uint32_t ctrl;
}GPIOregs;
#define GPIO ((GPIOregs*)GPIOBase)
typedef struct{
	uint32_t Out;
	uint32_t OE;
	uint32_t In;
	uint32_t InSync;
}rioregs;
#define rio ((rioregs *)RIOBase)
#define rioXOR ((rioregs *)(RIOBase + 0x1000 / 4))
#define rioSET ((rioregs *)(RIOBase + 0x2000 / 4))
#define rioCLR ((rioregs *)(RIOBase + 0x3000 / 4))
uint32_t pin = 20;
uint32_t fn = 5;

static int device_open(struct inode *inode, struct file *file){
    //static int counter = 0;
    //sprintf(Message, "counter=%d\n", counter++);
    //msg_Ptr = Message;

	GPIO[pin].ctrl=fn;
	pad[pin] = 0x10;//4mA
	rioSET->OE = 1 << pin;
	rioSET->Out = 1 << pin;
    msleep(1000);

    return 0;
}
static int device_release(struct inode *inode, struct file *file){
    rioCLR->Out = 1 << pin;   // 先變 LOW
    rioCLR->OE  = 1 << pin;   // 再關閉 output enable
	return 0;
}
static ssize_t device_read(struct file *filp, char *buffer, size_t length, loff_t *offset){
    bSleep=0;
    wait_event_interruptible(wq, bSleep);//ProcessA 注意:每次read都會卡住
    
    //buffer 和 length 來自 user space；但 filp、offset 是 kernel 幫你轉換/準備好的。
    if (*msg_Ptr == 0)return 0;
    size_t len = strlen(msg_Ptr);
    if (len > length) len = length;
    copy_to_user(buffer, msg_Ptr, len);
    msg_Ptr += len;
    return len;
}
static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t *off){
    int i;
    for (i=0; i < len; i++) {
        get_user(Message[i], buff + i);
    }
    Message[i] = '\0';
    msg_Ptr = Message;

    bSleep=1;
    wake_up_interruptible(&wq);//ProcessB

    struct task_struct *p;
    p=pid_task(find_vpid(user_pid), PIDTYPE_PID);
    send_sig( SIGUSR1, p, 0);//使用者自訂信號1

    return i;
}

static int __init hello_module(void){
    printk(KERN_EMERG "%s\n",mystring);
    slave_test();
    request_module("request");//request.c:14

    if(alloc_chrdev_region(&devno, 0 , 1 ,DEVICE_NAME) < 0){//已指定baseminor
        printk ("alloc_chrdev_region() fail\n");
        return -1;
    }
    Major=MAJOR(devno);
    cdev_init(&mycdev , &fops);
    if(cdev_add(&mycdev, devno , 1)){
        printk ("Error adding cdev\n");
    }
    printk("I was assigned major number %d.\n", Major);

    charmodule_class = class_create("charmodule_class");//設備類別
    device_create(charmodule_class, NULL, devno, NULL, "pico_sensor");//設備節點

	PERIBase = ioremap(0x1f00000000, 64 * 1024 *1024);
	if(PERIBase == NULL)printk("ioremap() fail\n");
	GPIOBase = PERIBase + 0xD0000 / 4;
	RIOBase = PERIBase + 0xE0000 / 4;	
	PADBase = PERIBase + 0xF0000 / 4;
	pad = PADBase + 1;
	printk("PERIBase:%px\n", PERIBase);	
	printk("GPIOBase:%px\n", GPIOBase);
    printk("RIOBase:%px\n", RIOBase);
    printk("PADBase:%px\n", PADBase);
    printk("pad:%px\n", pad);


    return 0;
}

static void __exit bye_module(void){
    printk("Bye\n");
    cdev_del(&mycdev);

    device_destroy(charmodule_class, devno);
    class_destroy(charmodule_class);

    unregister_chrdev_region(devno , 1);
    printk("unregister_chrdev\n");

	iounmap(PERIBase);
}

module_init(hello_module);
module_exit(bye_module);
MODULE_LICENSE("GPL");//宣告為開源
MODULE_AUTHOR("aacs20201128@gmail.com");
MODULE_DESCRIPTION("custom kernel space module");
// put_user(x, ptr)：適合放「單一簡單值」，例如 char、int。
// int bytes_read = 0;
// while (length && *msg_Ptr) {
//     put_user(*(msg_Ptr++), buffer++);//往下一個 byte 移。
//     length--;bytes_read++;
// }
// return bytes_read;

// copy_to_user(dst, src, n)：適合複製一整塊 buffer / struct。
// 如果資料只是「一串 bytes」，不一定需要 '\0'；但只要你後面要把它當 C 字串，
//  例如用 strlen()，就必須確保最後有 '\0'。
//  文字資料：補 '\0' 很安全，重複補也沒關係。
// if (len > length) len = length;防止kernel複製的資料量超過userspace這次read()指定的buffer長度。
// msg_Ptr += len;因為一次 read() 不一定能把 kernel 裡的資料全部拿走。讓下一次接著讀剩下的bytes。


//在 Linux 核心中，所有的應用程式（行程 Process）都是用一個名為 task_struct 的龐大資料結構來管理的
// struct task_struct *p;
// p=pid_task(find_vpid(user_pid), PIDTYPE_PID); 
// 找到對應的 task_struct
// 把數字 PID 轉成 struct pid *
// PIDTYPE_PID 表示：把這個 PID 當作一般 task PID 來找。


//Process A 進入 driver 的 read()，執行到wait_event_interruptible()時因為bSleep == 0而睡眠；
// 之後另一個kernel執行路徑，例如Process B 呼叫 write()，把 bSleep 設成 1 並
// 呼叫 wake_up_interruptible()，於是 Process A 被喚醒並繼續執行，
// 從它原本睡著的那個等待點附近繼續，不是從 read() 最前面重新開始。
// 不等於「A 現在立刻開始跑」，比較像把 A 從「睡覺中」叫醒，丟回「等 CPU 執行」的隊伍裡。
// 另外，Process B 呼叫 wake_up_interruptible(&wq) 之後，B 自己通常會繼續往下跑
// bSleep 是全域共用的一個旗標。所以一次 echo 很可能就把多個卡在同一個 wq 的 cat 都叫醒。
//如果示範 mutex，就用 mutex_lock() 等待；
// 如果示範 wait queue，就用 wait_event_interruptible() + bSleep 等待。

//每個直接指向硬體暫存器的指標，都應保留適當的 MMIO 存取語意。
//  在一般 C / user space 中，這類 mmap() 後取得的 MMIO 指標通常會宣告成 volatile，
//  避免編譯器把對硬體暫存器的讀寫最佳化掉。
//  在 Linux kernel 中，MMIO 位址通常會另外標記成 __iomem，以區分一般記憶體指標。

//sprintf() 和 sscanf() 是把原本 printf() / scanf() 對「終端機」做的事情，改成對「字串」做。

//Linux kernel 為了避免洩漏 kernel pointer，%p 預設會 hash pointer。
//  要看真正的 pointer，改成 %px

//mmap() 是讓 user space 程式把某段檔案或實體記憶體對應到自己的虛擬位址空間；
//  mmap() 不只用來映射 MMIO，也可以映射一般檔案；
//ioremap() 則主要就是給 kernel 映射 I/O/MMIO register 用的。