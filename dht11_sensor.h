#ifndef DHT11_H //這叫 include guard（標頭檔保護），作用是避免同一個 .h 被重複展開。
#define DHT11_H //這個 header 在同一次編譯中只展開一次。

#define DEVICE_FILE_NAME "/dev/dht11_sensor"
#define DEVICE_NAME "dht11_sensor"

#ifdef __KERNEL__  //// 只有在 Kernel 環境編譯時才使用這段程式碼
#include <linux/types.h>
#include <linux/io.h>

void __iomem *PERIBase;
void __iomem *GPIOBase;
void __iomem *RIOBase;

void __iomem *PADBase;
void __iomem *pad;

u32 pin = 21;
u32 fn = 5;

/* GPIO 十進制*/ 
#define GPIO_STRIDE          0x08  //步幅      
#define GPIO_STATUS_OFFSET   0x00
#define GPIO_CTRL_OFFSET     0x04
/* pad */
#define PAD_STRIDE           0x04
/* RIO */
#define RIO_OUT_OFFSET       0x00
#define RIO_OE_OFFSET        0x04
#define RIO_IN_OFFSET        0x08
#define RIO_INSYNC_OFFSET    0x0C
#define RIO_XOR_OFFSET       0x1000
#define RIO_SET_OFFSET       0x2000
#define RIO_CLR_OFFSET       0x3000

#endif
#endif
//.h 不是不能放 static，而是普通的 .c 私有函式通常不要放 .h；static inline 才很常放 header。

// static 變數放 .c → 這個 .c 私有
// static 變數放 .h → 每個 include 它的 .c 各自一份
// extern 變數放 .h → 宣告「真正的變數在別處」
// 真正的全域變數定義 → 通常只放在一個 .c

// static 函式放 .c → 這個函式只給這個 .c 使用，其他 .c 不能呼叫
// static 函式宣告放 .h → 每個 include 它的 .c 都會把它當成自己的 private 函式宣告
//  → 一般不建議這樣做，除非有特殊用途
// static 函式實作放 .h → 每個 include 它的 .c 都會各自產生一份函式
//  → 一般常見於 static inline
// 一般函式宣告放 .h → 讓其他 .c 知道這個函式存在，可以跨 .c 呼叫
// 一般函式實作 → 通常只放在一個 .c

//全放.h在include到.c不是很安全嗎
//問題是：這會讓 dht11.h 變得很「重」。任何其他 .c 只要 include dht11.h，就會被迫一起 include 一堆它根本不需要的東西。

//因為 __iomem 只是「標記」，它不能取代真正的 C 型別，所以前面還是要有 void 或其他型別。

// 不需要 / 4。
//  你以前用 u32 * 時 /4 是因為 pointer arithmetic 會一次跳 4 bytes；
//  現在 void __iomem * 配 MMIO offset，通常直接寫 byte offset。