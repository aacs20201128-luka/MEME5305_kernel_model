#include <stdio.h>      // printf(), perror() 等標準輸入輸出函式
#include <stdlib.h>     // exit(), malloc(), free() 等函式
#include <fcntl.h>      // open()、O_RDONLY、O_WRONLY、O_RDWR
#include <unistd.h>     // read(), write(), close() 等系統呼叫 、pause()
#include <sys/ioctl.h>  // ioctl() 裝置控制函式
#include <string.h>     // strlen(), strcpy(), strcmp(), memset() 等字串處理函式
#include <signal.h>     // userspace signal 定義與處理函式，如 signal()、SIGUSR1

#include <stdint.h>     // 提供固定寬度整數型別，例如 uint32_t、uint64_t
#include <errno.h>      // 提供 errno 與錯誤碼，例如 EINVAL、EACCES
#include <sys/mman.h>   // 提供 mmap()、munmap()、PROT_*、MAP_* 等記憶體映射功能

typedef struct{
    uint32_t status; // 4 bytes
    uint32_t ctrl;   // 4 bytes
}GPIOregs;           // 所以一個 GPIOregs 總共是：8 bytes
typedef struct{      //要看 datasheet 的 register width 和 offset。
    uint32_t Out;    // RIO 這裡每個 register 間隔 4 bytes，所以是 32-bit。
    uint32_t OE;
    uint32_t In;
    uint32_t InSync;
}rioregs;
#define GPIO ((GPIOregs*)GPIOBase) //把 GPIOBase 這個位址，轉成「指向 GPIOregs 的指標」。

//兩者剛好差：0x4000 bytes 一組 RIO 佔 0x4000 bytes，因為硬體幫它保留了 4 個各 0x1000 的操作區。
//sys_rio0 = 0x400E0000
//sys_rio1 = 0x400E4000
#define rio ((rioregs *)RIOBase)   //rio 只是做「文字替換」，不是立刻執行。
#define rioXOR ((rioregs *)(RIOBase + 0x1000 / 4))//拿原本 bit 跟 mask bit 比，不同得 1，相同得 0。
#define rioSET ((rioregs *)(RIOBase + 0x2000 / 4))//mask 中指定的 bit 被 set
#define rioCLR ((rioregs *)(RIOBase + 0x3000 / 4))//mask 中指定的 bit 被 clear

int main(int argc, char *argv[]){
    //mmap就是先把「RP1 整棟大樓的入口」映射進你的 userspace。
    int memfd = open("/dev/mem", O_RDWR | O_SYNC);
    uint32_t *map = (uint32_t *)mmap(          // 呼叫 mmap()，並把回傳位址轉成 uint32_t * 指標
        NULL,                                  // 讓核心自動選擇虛擬位址
        64 * 1024 * 1024,                      // 映射 64 MiB 的記憶體範圍
        (PROT_READ | PROT_WRITE),              // 映射區域允許讀取與寫入
        MAP_SHARED,                            // 使用共享映射，寫入會反映到實體裝置/記憶體
        memfd,                                 // /dev/mem 的檔案描述符
        0x1f00000000                           // 要映射的實體記憶體起始位址
    );                                         // mmap() 呼叫結束
    if (map == MAP_FAILED){
        printf("mmap failed: %s\n", strerror(errno));
        return (-1);
    }
    close(memfd);
    uint32_t *PERIBase = map;// 周邊暫存器映射區域的基底位址
    uint32_t *GPIOBase = PERIBase + 0xD0000 / 4;//GPIO暫存器基底位址；除以4是因為uint32_t每格4bytes
    uint32_t *RIOBase  = PERIBase + 0xE0000 / 4;//RIO暫存器基底位址;上面有4個同一組RIO暫存器的不同操作入口
    uint32_t *PADBase  = PERIBase + 0xF0000 / 4;//PAD暫存器基底位址
    uint32_t *pad      = PADBase + 1;//這裡 +1 就是跳過 Offset 0x00 的 VOLTAGE_SELECT Register。
    printf("PERIBase:%p\n", PERIBase);
    printf("GPIOBase:%p\n", GPIOBase);
    printf("RIOBase:%p\n", RIOBase);
    printf("PADBase:%p\n", PADBase);

    uint32_t pin = 20;  // GPIO20
    uint32_t fn = 5;    // SYS_RIO

    GPIO[pin].ctrl=fn;  //選擇腳位功能。在 C 裡，指標本來就可以用陣列語法存取。 等價於: *(GPIO + pin)
    pad[pin] = 0x10;    //C 可以把指標用陣列索引語法來操作。 *(pad + pin)

    //所以 1u << pin 這個寫法，真正依據不是「RIO 章節有直接列 bit N = GPIO N」，
    // 而是來自 RP1 GPIO bank 的整體設計慣例與 Linux driver 實作：
    //  GPIO bank 0 是按 pin 編號對應 bit 位置來操作的。
    //  規格書其他 GPIO 相關暫存器也明確列出像 bit 0 = GPIO0、bit 1 = GPIO1 … bit 27 = GPIO27 這種排列。
    rioSET->OE = 1 << pin; //這裡是給你設定bitmask的!不是用來自己運算的
    rioSET->Out = 1 << pin;

    for (;;)
    {
        rioXOR->Out = 1 << pin; // Toggle the GPIO20
        sleep(1);
    }

    //程式正常退出時 Linux 會幫你收；程式長時間運作、反覆映射時，就應該自己 munmap()。
    munmap(map, 64 * 1024 * 1024);//解除映射以釋放資源。
    return 0;
}

// 開啟 /dev/mem，並用 mmap() 將實體硬體暫存器所在的實體位址範圍，
//  映射到 userspace 應用程式的虛擬記憶體空間中。
// 之後程式透過 GPIOBase、RIOBase 這些指標，實際上就是在讀寫對應的 MMIO 周邊暫存器。
// 因為你的程式是跑在 BCM2712 的 CPU 上，不是跑在 RP1 上。
//  真正執行這行指令的是 BCM2712 裡面的 ARM CPU。
//  但 GPIO 控制器在 RP1 裡。所以 CPU 必須「跨出去」跟 RP1 溝通。
//  CPU 想改 GPIO，就像主機送命令給 I/O 控制中心。
// PCIe 可以先把它想成：BCM2712 和 RP1 之間的一條高速通道。
//  不是說你一定插了一張 PCIe 卡，而是 SoC 內部/板子上的晶片之間，也可以用 PCIe 連接。
//  CPU 透過這條 PCIe 連線，去讀寫 RP1 裡的暫存器。

// RP1 自己看自己的 GPIO，會說：我的 GPIO 在 0x400D0000 這是 RP1 內部的地址。
//  但 BCM2712 從外面看 RP1，不能直接使用 RP1 內部那套地址。
//      BCM2712 會把「RP1 的整塊空間」放到自己實體位址空間中的某個區域。
//  BCM2712 看 RP1 的起點：0x1F00000000

// 在 Raspberry Pi 5 上，BCM2712 的 Linux kernel 也不能把 
//  RP1 datasheet 的 0x400D0000 當成 CPU 的實體位址直接存取。
//      關鍵差別是：Pico 的情況和 Pi 5 + RP1 的架構不同。
// 在 Pico / RP2040 裡，ARM Cortex-M0+ CPU 和 GPIO 周邊就在同一顆 RP2040 裡、共用同一套 address map
// 因為對 RP2040 CPU 而言：0x40014000 就真的在它自己的 address space 裡。
// Pico：CPU 就住在周邊旁邊，datasheet 地址就是 CPU 可以使用的地址。

// Userspace 的虛擬記憶體彼此隔離；Kernel space 則是整個系統共用的一套核心位址空間。
// Userspace 和 Kernel 平常都是用「虛擬位址」在操作記憶體。
//  在 kernel driver 裡，通常先用 ioremap() 把這段 MMIO physical address 映射成 kernel virtual address。
//      之後 kernel 用 readl()、writel() 去讀寫那個虛擬位址。
// MMIO 雖然對應到實體硬體位址，但通常都會先映射成虛擬位址再存取，不會直接拿實體位址當普通指標用。
//  uint32_t *gpio = (uint32_t *)0x1F000D0000;C 本身雖然允許你寫，但 CPU 會把 0x1F000D0000 當成虛擬位址去查頁表；
// 0x1F00000000 可以看成是 BCM2712 這一側的 physical address space 裡，
//  一段通往 RP1 的 MMIO 位址區域，不一定是「RAM」。
// 可以把每個元件想成「都有自己看世界的地址地圖」：
//  RP1 有 RP1 的地址，BCM2712 有 BCM2712 的地址，中間靠 PCIe 做對應。
// 最簡單一句：
//  地址是「站在哪個元件的角度看」而定；同一個 GPIO，在 RP1 看是一個地址，在 BCM2712 看可能是另一個地址。

// MMIO 的重點不是「用位址存取」這件事很特別，而是：
//  某些記憶體位址其實不是 RAM，而是被硬體拿來代表 I/O 裝置的暫存器。
// value = *addr;   // 如果 addr 指向 RAM，就是讀記憶體
// value = *gpio;   // 如果 gpio 指向 MMIO，就是讀 GPIO 暫存器
// CPU 指令看起來可能很像，但硬體上：
// RAM address   → 去找記憶體
// MMIO address  → 去找 GPIO / UART / SPI 等硬體
// 這也是為什麼叫 Memory-Mapped I/O ;  I/O 被「映射進記憶體地址空間」裡。

// 對，你現在用 mmap() 了，但 volatile 仍然有意義，只是它跟 mmap() 解決的是不同問題。
//  mmap() 解決的是：把 MMIO 的實體位址映射成 userspace 可以用的虛擬位址。
//  volatile 解決的是：告訴 C 編譯器：這塊位址是硬體暫存器，不要把讀寫最佳化掉。
//  所以即使你已經 mmap()，MMIO 指標通常還是會用 volatile。
// 重點是要加誰? 只要這個指標指向的是 MMIO 硬體暫存器，通常就應該保留 volatile。
//  volatile 不是因為「虛擬位址」，而是因為「那個位址後面接的是硬體，硬體可能自己變」。
// 所以 volatile 的意思就是：這個位址裡的值可能不是程式改的，而是硬體自己改的。
//  如果硬體會自己改，那你手上的舊值就可能已經不準了。
//  位址通常固定，變的是那個硬體暫存器裡的 bit / 數值。
//  硬體會自己改值時，自己做 read-modify-write 可能有競爭問題；SET/CLR/XOR 就是用來避免這種情況。

//(/dev/mem)：代表系統的實體記憶體介面。存取 MMIO 周邊暫存器時，常會透過它再搭配 mmap()。
//O_SYNC：要求同步 I/O，避免某些寫入被延後或快取。

//sudo cat /proc/iomem  ;Linux 目前如何分配整個實體位址空間。
//  起始位址-結束位址 : 這段位址目前給誰使用
//sudo cat /sys/kernel/debug/gpio | grep 1f ;GPIO 線目前被誰使用、方向、電位狀態

// 例如:uint32_t *GPIOBase = PERIBase + 0xD0000 / 4;
//  不(/4 )就會走太遠 4 倍。因為 PERIBase 是 uint32_t* 每 +1 走 4 bytes，所以除以 4 算步數
//  如果想用「byte offset」直接算位址，char * 其實很好用。char *PERIBase = (char *)map;這時就不用 /4。
//  都是「存位址的變數」，型別不同只是決定「這個位址要怎麼被解讀」。

// 1u 型別是 unsigned int
//  32-bit int：1 << 0 到 1 << 30 比較安全；要碰 bit31 用 1u << 31。
//  低位通常沒差，做 bitmask 習慣加 u 比較保險。