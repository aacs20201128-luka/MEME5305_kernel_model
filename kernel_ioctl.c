#include <linux/version.h>    // Linux Kernel 版本資訊
#include <linux/module.h>     // Kernel Module
#include <linux/kernel.h>     // Kernel 核心功能
#include <linux/fs.h>         // 檔案操作 file_operations
#include <linux/init.h>       // module_init / module_exit
#include <linux/major.h>      // 主要是放 Linux 已知／保留的 Major Number 定義。
#include <linux/device.h>     // Device / Class
#include <linux/poll.h>       // poll 機制
#include <linux/io.h>         // MMIO：ioremap/readl/writel
#include <linux/delay.h>      // 延遲函式
#include <linux/uaccess.h>    // User/Kernel 資料傳遞
#include <linux/cdev.h>       // Character Device
#include <linux/mutex.h>      // Mutex 互斥鎖

#include "kernel_ioctl.h"        // 自訂 ioctl 指令

#define DEVICE_NAME "kernel_ioctl_led"
static int Major;
static struct class *charmodule_class; 

static DEFINE_MUTEX(mlock);

volatile uint32_t *PERIBase;
volatile uint32_t *GPIOBase;
volatile uint32_t *RIOBase;
volatile uint32_t *PADBase;
volatile uint32_t *pad;
typedef struct{
    uint32_t status;
    uint32_t ctrl; 
}GPIOregs;
typedef struct
{
    uint32_t Out;
    uint32_t OE;
    uint32_t In;
    uint32_t InSync;
} rioregs;
#define GPIO ((GPIOregs*)GPIOBase)
#define rio ((rioregs *)RIOBase)
#define rioXOR ((rioregs *)(RIOBase + 0x1000 / 4))
#define rioSET ((rioregs *)(RIOBase + 0x2000 / 4))
#define rioCLR ((rioregs *)(RIOBase + 0x3000 / 4))

uint32_t pin = 20;
uint32_t fn = 5;
int led_on_off = 0;

static int led_open(struct inode *inode, struct file *file){
  return 0;
}
static int led_release(struct inode *inode, struct file *file){
  return 0;
}
static ssize_t led_read(struct file *filp, char *buffer, size_t length, loff_t *offset){
  int bytes_read = 0;
  char prbuf[80];

  sprintf(prbuf, "%d", led_on_off);
  if ( copy_to_user(buffer, prbuf, strlen(prbuf)) !=0 ) {
    printk("copy_to_user() fail\n");
    return -EFAULT;
  }
  bytes_read++;
  return bytes_read;
}
static ssize_t led_write(struct file *filp, const char *buff, size_t len, loff_t *off){
  int bytes_written=0;
  char pwbuf[80];

  if ( copy_from_user(pwbuf, buff, len) !=0 ) {
    printk("led_write(): len:%lu\n", len);
    return -EFAULT;
  }
  sscanf(pwbuf,"%d",&led_on_off);
  mutex_lock(&mlock);
  printk(KERN_ALERT "led_write()led_on_off:%d\n", led_on_off);
  if ( led_on_off ) {
    writel( 1<<pin, &rioSET->Out);
    led_on_off = 1;
  }
  else {
    writel(1<<pin, &rioCLR->Out);
    led_on_off = 0;
  }
  mutex_unlock(&mlock);

  bytes_written++;
  return bytes_written;
}

static long led_ioctl(struct file *file,unsigned int ioctl_num, unsigned long ioctl_param) {
  mutex_lock(&mlock);
  switch (ioctl_num) {
    case IOCTL_SET_LED_ON:
      printk(KERN_ALERT "IOCTL_SET_LED_ON");
      writel(1<<pin, &rioSET->Out);
		  led_on_off = 1;
      break;
    case IOCTL_SET_LED_OFF:
      printk(KERN_ALERT "IOCTL_SET_LED_OFF");
      writel(1<<pin, &rioCLR->Out);
      led_on_off = 0;
      break;
    case IOCTL_TOGGLE_NTH_LED:
      printk(KERN_ALERT "IOCTL_TOGGLE_NTH_LED");
      writel(1<<pin, &rioXOR->Out);
      led_on_off ^= 1;
      break;
    default:
      printk(KERN_ALERT "Unknown IOCTL command\n");
  }
  mutex_unlock(&mlock);
  return 0;
}

static struct file_operations fops = {
    .read = led_read,
    .write = led_write,
    .unlocked_ioctl = led_ioctl,
    .open = led_open,
    .release = led_release,
};

static int __init hello_module(void)
{
    Major = register_chrdev(0, DEVICE_NAME, &fops);//舊版 API
    if (Major < 0) return -1;
    printk("I was assigned major number %d.\n", Major);

    charmodule_class = class_create(DEVICE_NAME); // 建立一個設備類別
    if(IS_ERR(charmodule_class)) return -1;
    device_create(charmodule_class, NULL, MKDEV(Major, 0), NULL, "kernel_ioctl");//建立設備節點
    printk(KERN_ALERT"'mknod /dev/kernel_ioctl c %d 0'.\n", Major);

    PERIBase = ioremap(0x1f00000000, 64 * 1024 * 1024);
    GPIOBase = PERIBase + 0xD0000 / 4;
    RIOBase = PERIBase + 0xe0000 / 4;
    PADBase = PERIBase + 0xf0000 / 4;
    pad = PADBase + 1 ;
    led_on_off = 0;

    printk("PERIBase:%p\n", PERIBase);    
    printk("GPIOBase:%p\n", GPIOBase);    
    printk("RIOBase:%p\n", RIOBase);    
    printk("PADBase:%p\n", PADBase);
    printk("pad:%p\n", pad);

    writel(fn , &GPIO[pin].ctrl);
    writel( 1 , &pad[pin]);
    writel( 1 << pin , &rioSET->OE);
    if( led_on_off != 0 )
      writel( 1 << pin , &rioSET->Out);
    return 0;

    fail_cdev_add:
    fail_register_chrdev:
    return -1;
}

static void __exit bye_module(void)
{
    //u32 val = readl(&rio->OE);//OE Register 整個 32-bit 值
    writel( 1 << pin , &rioCLR->OE);//只是「計算 AND 的結果」，不會改變 val。

    iounmap(PERIBase);

    device_destroy(charmodule_class, MKDEV(Major, 0)); 
    class_destroy(charmodule_class); 

    unregister_chrdev(Major, DEVICE_NAME);

    printk("unregister_chrdev\n");
}

module_init(hello_module);
module_exit(bye_module);
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("aacs20201128@gmail.com");
MODULE_DESCRIPTION("A minor number demo");


// MKDEV(ma, mi)：把主編號 major 和次編號 minor 組成一個完整的裝置編號 dev_t。
// MAJOR(devno)：從完整的裝置編號 dev_t 裡，把 major 主編號取出來。

// u32 是 Linux Kernel 常用的 32-bit 無號整數型別。
//  你可以把它理解成 Kernel 版本的固定寬度整數型別，概念上類似 user space 的：
//  uint32_t

// u32 readl(const volatile void __iomem *addr);
// void writel(u32 value, volatile void __iomem *addr);
// readl(暫存器位址);           // 從 MMIO Register 讀 32-bit 資料 
//  回傳值：讀到的 u32，也就是 32-bit 無號整數
// writel(要寫的值, 暫存器位址); // 寫 32-bit 資料到 MMIO Register 沒有回傳值

// __iomem 可以先理解成：這個指標是指向硬體 MMIO 區域，不是一般 RAM。

// static DEFINE_MUTEX(mlock);   // 建立一把互斥鎖 mlock
// mutex_lock(&mlock);           // 上鎖：其他人要等，只有一個執行緒能進入
// mutex_unlock(&mlock);         // 解鎖：做完後讓其他人進入
// 跟 C#：概念幾乎一樣，都是用來避免多個執行緒同時修改同一份資料。差別是 Linux Kernel 的 mutex 
//  要自己明確 lock / unlock。

// #define rioSET ((rioregs *)(RIOBase + 0x2000 / 4)) 型別是：指向 rioregs 結構的指標
// rioSET->Out    = Out 暫存器本身
// &rioSET->Out   = Out 暫存器的位址 ← writel() 要這個


// 如果 rioSET 真的是指向 SET alias 區塊，那它主要用途確實是「寫入 1 來把對應 bit 設成 1」，不
//  是拿來當一般狀態暫存器讀。RP1 datasheet 說明 RIO_OUT、RIO_OE 本身有 SET / CLEAR / XOR 
//  alias，這些 alias 主要是給 atomic write 使用。
//  比較合理的讀法應該是讀「一般 RIO 區塊」的 OE：
//    val = readl(&rio->OE);       // 讀目前整個 OE 狀態
// SET / CLR / XOR 的特殊效果只發生在「write」；
// read 仍然可以正常讀到原本 register 的值。
// 這四種位址在 read 時都會正常讀到同一個 register 的值。
// 所以可以說：後三種都是透過不同 alias 位址，去操作 0x400E0000 這組 RIO register 所代表的同一
//  份硬體狀態。
/// SET / CLR / XOR 都只看 mask 哪個 bit = 1：SET 設 1、CLR 清 0、XOR 反轉；不要去想背後的位
///   元運算算法。