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

#include <linux/platform_device.h>  // 使用 Linux Platform Device / Platform Driver 架構
#include <linux/of.h>               // 使用 Device Tree / OF（Open Firmware）配對功能
#include <linux/clk.h>              // 使用 Linux Clock Framework，取得、啟用與查詢硬體時脈
static struct clk *uart_clk;        // 宣告一個 Clock 物件指標，用來保存 UART 對應的時脈

#include "./pico_sensor.h"

static dev_t devno=0;
static struct cdev mycdev;
static int Major=0;
static char Message[80];
static int pico_sensor_open(struct inode *, struct file *);
static int pico_sensor_release(struct inode *, struct file *);
static ssize_t pico_sensor_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t pico_sensor_write(struct file *, const char __user *, size_t, loff_t *);
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = pico_sensor_read,
    .write = pico_sensor_write,
    .open = pico_sensor_open,
    .release = pico_sensor_release
};
static struct class *charmodule_pico_class;

static int pico_sensor_open(struct inode *inode, struct file *file){
  	// writel( 0x58 , (u8 __iomem *)pad + PAD_STRIDE * pin);//開IE、4mA、開Pull up 
    // writel( 1 << pin , (u8 __iomem *) RIOBase + RIO_SET_OFFSET + RIO_OE_OFFSET);
    // writel( 1 << pin , (u8 __iomem *) RIOBase + RIO_SET_OFFSET + RIO_OUT_OFFSET);
    // msleep(1000);
    return 0;
}
static int pico_sensor_release(struct inode *inode, struct file *file){
    // writel( 1 << pin , (u8 __iomem *) RIOBase + RIO_CLR_OFFSET + RIO_OUT_OFFSET);
    // writel( 1 << pin , (u8 __iomem *) RIOBase + RIO_CLR_OFFSET + RIO_OE_OFFSET);
	  return 0;
}
static ssize_t pico_sensor_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset){
    //u32 value = readl((u8 __iomem *)UART0_Base + UART_CR_OFFSET);
    int i = 0;

    // 先清掉 RX FIFO 裡目前殘留的舊資料
    while (!(readl((u8 __iomem *)UART0_Base + UART_FR_OFFSET) & (1 << 4))) {
        readl((u8 __iomem *)UART0_Base + UART_DR_OFFSET);
    }

    while(i < sizeof(Message)){
        int timeout = 1000000;
        while (readl((u8 __iomem *)UART0_Base + UART_FR_OFFSET) & (1 << 4)) { // RXFE = 1
            if (--timeout == 0) {
                printk("UART RX timeout\n");
                return -ETIMEDOUT;
            }
            udelay(1);
        }

        Message[i] = readl((u8 __iomem *)UART0_Base + UART_DR_OFFSET) & 0xFF;
        if(Message[i] == '\n'){
            i++;
            break;
        }
        i++; 
    }
    //Message[i] = '\0';
    size_t len;
    len = min(length, (size_t)i);
    if (copy_to_user(buffer, Message, len))
        return -EFAULT;
    //*offset += len;
    return len;
}
static ssize_t pico_sensor_write(struct file *filp, const char __user *buff, size_t len, loff_t *off){
    return 0;
}

static int pico_sensor_probe(struct platform_device *pdev){
    int ret;

    printk("pico_sensor_probe entered\n");

    uart_clk = devm_clk_get(&pdev->dev, "uartclk");

    if (IS_ERR(uart_clk)) {
        printk("devm_clk_get uartclk failed\n");
        return PTR_ERR(uart_clk);
    }

    ret = clk_prepare_enable(uart_clk);

    if (ret) {
        printk("clk_prepare_enable failed\n");
        return ret;
    }

    printk("UART clock = %lu Hz\n", clk_get_rate(uart_clk));

    writel( fn , (u8 __iomem *)GPIOBase + GPIO_CTRL_OFFSET +  GPIO_STRIDE * uart0_txd);
    writel( fn , (u8 __iomem *)GPIOBase + GPIO_CTRL_OFFSET +  GPIO_STRIDE * uart0_rxd);

    writel( 0x000  , (u8 __iomem *)UART0_Base + UART_CR_OFFSET);//關閉 RXE、TXE、UARTEN
    writel( 27, (u8 __iomem *)UART0_Base + UART_IBRD_OFFSET); // UARTIBRD  26
    writel( 8,  (u8 __iomem *)UART0_Base + UART_FBRD_OFFSET); // UARTFBRD  3
    writel( 0x70 , (u8 __iomem *)UART0_Base + UART_LCR_H_OFFSET); // UARTLCR_H
    writel( 0x301  , (u8 __iomem *)UART0_Base + UART_CR_OFFSET);//UARTCR 開啟


    printk("GPIO14_CTRL = 0x%x\n",readl((u8 __iomem *)GPIOBase + GPIO_CTRL_OFFSET + GPIO_STRIDE * 14));
    printk("GPIO15_CTRL = 0x%x\n",readl((u8 __iomem *)GPIOBase + GPIO_CTRL_OFFSET + GPIO_STRIDE * 15));
    printk("UARTCR = 0x%x\n",readl((u8 __iomem *)UART0_Base + UART_CR_OFFSET));
    printk("UARTIBRD = %u\n", readl((u8 __iomem *)UART0_Base + UART_IBRD_OFFSET));
    printk("UARTFBRD = %u\n",readl((u8 __iomem *)UART0_Base + UART_FBRD_OFFSET));
    printk("UARTLCR_H = 0x%x\n",readl((u8 __iomem *)UART0_Base + UART_LCR_H_OFFSET));
    printk("UARTFR = 0x%x\n",readl((u8 __iomem *)UART0_Base + UART_FR_OFFSET));
    return 0;
}
static const struct of_device_id pico_sensor_of_match[] = {
    { .compatible = "aacs,pico-uart" },
    { }
};
static struct platform_driver pico_sensor_platform_driver = {
    .probe = pico_sensor_probe,

    .driver = {
        .name = "pico_sensor",
        .of_match_table = pico_sensor_of_match,
    },
};

static int __init pico_sensor_init(void){
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

    charmodule_pico_class = class_create("charmodule_pico_class");//設備類別
    device_create(charmodule_pico_class, NULL, devno, NULL, DEVICE_NAME);//設備節點

    PERIBase = ioremap(0x1f00000000, 64 * 1024 * 1024);
    if(PERIBase == NULL)printk("ioremap() fail\n");
    GPIOBase = (u8 __iomem *)PERIBase + 0xD0000;
    RIOBase = (u8 __iomem *)PERIBase + 0xE0000;	
    PADBase = (u8 __iomem *)PERIBase + 0xF0000;
    pad = (u8 __iomem *)PADBase + 0x04;
    UART0_Base = (u8 __iomem *)PERIBase + 0x30000;
    printk("PERIBase:%px\n", PERIBase);	
    printk("GPIOBase:%px\n", GPIOBase);
    printk("RIOBase:%px\n", RIOBase);
    printk("PADBase:%px\n", PADBase);
    printk("pad:%px\n", pad);
    printk("UART0_Base:%px\n", UART0_Base);

    int ret;
    ret = platform_driver_register(&pico_sensor_platform_driver);
    if (ret) {
        printk("platform_driver_register failed\n");
        return ret;
    }

    return 0;
}
static void __exit pico_sensor_exit(void){
    platform_driver_unregister(&pico_sensor_platform_driver);

    cdev_del(&mycdev);

    device_destroy(charmodule_pico_class, devno);
    class_destroy(charmodule_pico_class);

    unregister_chrdev_region(devno , 1);
    printk("unregister_chrdev\n");

	iounmap(PERIBase);
}

MODULE_DEVICE_TABLE(of, pico_sensor_of_match);

module_init(pico_sensor_init);
module_exit(pico_sensor_exit);
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("aacs20201128@gmail.com");
MODULE_DESCRIPTION("Pico UART sensor driver");



//https://support.arm.com/documentation/ddi0183/g/programmers-model/summary-of-registers
//PrimeCell UART (PL011) 本身就是 ARM 的 UART Controller 規格文件，
// 所以裡面幾乎全部都在講 UART 的 register、FIFO、baud rate、interrupt、status。
//  PL011，它是 ARM 設計的一套 UART Controller IP，也可以理解成一種 UART 硬體控制器的標準設計。

// RP1 UART Baud Rate 計算
// UARTCLK = clk_uart = 48 MHz
// Baud Rate = 115200

// BAUDDIV = UARTCLK / (16 × Baud Rate)
// BAUDDIV = 48000000 / (16 × 115200)
// BAUDDIV = 26.041666...

// UARTIBRD = BAUDDIV 的整數部分
// UARTIBRD = 26

// UARTFBRD = round(BAUDDIV 的小數部分 × 64)
// UARTFBRD = round(0.041666 × 64)
// UARTFBRD = round(2.6666)
// UARTFBRD = 3

// #include <math.h> round() 就是「四捨五入」。
// 但這種 Baud Rate 計算通常不一定真的要呼叫 round()
// 所以看到 ARM 文件寫：integer((fraction × 64) + 0.5) 本質上就是在做四捨五入。

// FIFO 開啟：你看的 TXFF、RXFE 是在看「整個 FIFO」滿不滿、空不空。
// FIFO 關閉：UART 還是有基本的收發暫存機制，只是等同很淺的 buffer，
//  所以你仍然要判斷能不能寫、能不能讀。
// FF = Full
// FE = Empty
// TX：滿就等。RX：空就等，根本沒東西可以讀，所以只能等。

//要檢查 Empty，不是 Full。
//  因為你是在「接收資料」，你關心的是：RX FIFO 現在有沒有資料？
//  RXFE = 1  // RX FIFO 是空的，還沒資料
//  RXFE = 0  // RX FIFO 不是空的，有資料可以讀

//##########################################################################
///UART peripheral 做完之後，開始考慮「如果 RX 來不及處理怎麼辦」，正是從硬體介面進入通訊協議設計的下一步。
//  不是 CPU 先讀 UARTDR，UART 才開始收資料。
//  UARTDR 比較像是「CPU 用來把 RX FIFO 裡已經收到的資料拿出來」的資料暫存器介面。
//  如果 CPU 一直不讀 UARTDR，UART 還是會繼續收，直到 RX FIFO 滿掉。
//      FIFO 滿了之後對方還繼續送，就可能發生 RX overrun。
///所以你理解得沒錯：沒有 RTS/CTS 或軟體 flow control 時，TX 可以持續送，甚至把對方 RX 撐爆。
//  TX FIFO 的目的不是保護對方 RX，而是：
//  讓本機 CPU 可以先把資料排隊，UART 再按照 baud rate 慢慢送出去。
//  如果要避免把對方 RX FIFO 撐爆，就要另外靠 RTS/CTS、XON/XOFF，或自己設計 ACK/STOP 協議。

// 如果雙方都沒開 FIFO，UART 還是可以正常傳輸，但 TX/RX 幾乎只能一次處理 1 byte。
// RX 端收到資料後會先放進 RX data register；如果 CPU/driver 還沒讀走，下一個 byte 又到，
//  就可能發生 overrun 而丟資料。
// TX 端則需要更頻繁等待暫存器可寫，因此 CPU 負擔會比較高。
//  簡單說，沒有 FIFO 不是不能傳，而是容錯時間更短、效率也更差。
//##########################################################################

// *offset 記錄「同一次 open() 的檔案目前讀到哪個位置」


////你前面一直 timeout 的核心原因，也終於被實測證明：
// UART register 雖然有寫進去，但 clock 沒有被你的 driver 正式 enable。
//這個成果其實很漂亮，因為你不是用 /dev/ttyAMA0 直接收，而是真的做到：
// 自己的 Kernel Module 接管 RP1 UART0，最後由 /dev/pico_sensor 把資料送到 User Space。
// (/dev/ttyAMA0) 是 Linux 內建 PL011 UART Driver 已經幫你把硬體處理好了；
// (/dev/pico_sensor) 則是你自己寫 Kernel Module 去接管 UART0。

// 當 Linux 的 UART0 driver 被停用後，就沒有原本的 PL011 driver 幫 UART0 取得並 enable clk_uart。
// 我的自寫 UART module 雖然有依照 50MHz 重新計算 IBRD/FBRD，
// 但目前只是在操作 UART register，並沒有透過 Linux Clock Framework 取得並啟用 clk_uart，
// 所以 UART 仍可能無法正常工作。

//  Linux Clock Framework 是 Linux Kernel 用來統一管理硬體時脈 clock 的機制。
//  writel(27, UART_IBRD);這只是告訴 UART：如果有 clock 進來，要用什麼比例把它除成 115200 baud。
//  但你沒有做：向 Linux 要 clk_uart -> 告訴 Linux 我要使用它 -> 把 clock enable
//  正常 Linux Driver 會透過 Clock Framework 做：uart_clk = devm_clk_get(dev, "uartclk");
//  devm_clk_get(dev, "uartclk") // 向 Linux Clock Framework 取得這個裝置對應的 uartclk
//  clk_prepare_enable(uart_clk); // 準備並啟用 uartclk，讓硬體可以真正使用這個 clock
//  clk_get_rate(uart_clk);// 取得 uartclk 目前實際頻率，例如 50000000 Hz

// 硬體 clock 是晶片本身產生的，Linux 是透過 Clock Framework 去管理它的開關、來源、頻率與使用者。
//  所以不是「Linux 創造 clock」，而是：Linux 負責管理硬體既有的 clock。

// 這也是為什麼正常 Linux PL011 driver 不會把 48 MHz 寫死，
//  而是會向 Clock Framework 查「現在到底是多少 Hz」，再計算 IBRD/FBRD。
//  sudo cat /sys/kernel/debug/clk/clk_summary | head -2  #先看欄位名稱
//  sudo cat /sys/kernel/debug/clk/clk_summary | grep -i uart
//  如果是在 Kernel Driver 裡，最準的是：clk_get_rate(uart_clk) // 取得目前實際 UART clock，單位 Hz
//  clk_uart  ; 系統 Clock Provider 給它的名稱
//  1f00030000.serial ;使用這個 clock 的 RP1 UART0 device
//  uartclk ; UART0 driver 內部引用這顆 clock 時使用的名稱
//      uartclk 這個名字本來就存在於 Raspberry Pi 的 RP1 Device Tree 原始定義裡，
//      不是你剛剛 overlay 自己發明的。
//  正好對應你程式：devm_clk_get(&pdev->dev, "uartclk");//這個 "uartclk" 就是最後那一欄。
// 如果你換成 RP1 的 UART1、UART2，而且仍然是你自己的 Kernel Module 
//  直接接管那顆 UART hardware，那通常還是要處理 clock。
// devm_clk_get(&pdev->dev, "uartclk") 
//  取得「這個 UART 裝置」在 Device Tree 裡名為 uartclk 的 clock；
//  多個 UART 可以最後共用同一個底層 clk_uart。

// 你寫的 overlay → 改 compatible → 改 status
// 原本 rp1.dtsi → 已經定義 reg → interrupts → clocks → clock-names = "uartclk", "apb_pclk"

// pico-uart-overlay.dts   // 你寫的原始 Device Tree Overlay
//         ↓ dtc 編譯
// pico-uart.dtbo          // 編譯後的 binary overlay
//         ↓ 放進 overlays
// dtoverlay=pico-uart     // config.txt 載入它

// Device Tree        // 描述硬體在哪裡、有什麼資源
// platform_device    // Linux 根據 Device Tree 建立的裝置物件
// platform_driver    // 你寫來控制這個裝置的 Driver
//  Linux 專門管理「焊在板子上、系統本來就知道它在哪裡」的硬體的一種 Driver 架構。
// probe()            // 裝置和 Driver 配對成功後執行

// platform_driver ≠ /dev
// 你的 driver 是同時用了 Platform Driver Framework +
//  Character Device Framework，才有現在這個完整結果。

//「使用既有 UART driver」不代表「你不能再寫 Kernel Driver」；
// 只是你不要重寫底層 UART Controller Driver，而是寫上層的「Pico 裝置 Driver」。
// PL011 Driver = 快遞公司的司機，負責把包裹送來
// Pico Driver = 你公司的收貨人員，負責拆包裹、看內容、分類
// 你現在的專題是：連司機都自己當 + 收貨也自己做
// 上層 Pico 裝置 Driver 通常會用 serdev 技術。
//  Linux 內建 PL011 driver 負責「把 UART 資料收進來」，serdev driver 負責「看懂 Pico 傳來的資料」。
//  簡單需求：User Space 解析就好。

//RP1 UART0 = 真正的 UART 硬體 controller
//PL011 driver = Linux kernel 裡控制這顆 UART controller 的 driver，通常會被編譯進 Raspberry Pi Kernel 本體中。
//  它是一個 Linux Kernel driver；在 Raspberry Pi Kernel 裡通常是直接編譯進 Kernel 本體，而不是獨立 .ko module。
//      有印象了，kernel可以(*)編譯進 Kernel 本體 或 (m)編譯成 Kernel Module
//  然後 Linux 的 TTY framework 就會幫它管理成這種裝置:/dev/ttyAMA0
//uart0-pi5 = Raspberry Pi 的 Device Tree Overlay 
//  Pi 5 上這組 pin / UART0 要啟用，請把硬體配置好。
