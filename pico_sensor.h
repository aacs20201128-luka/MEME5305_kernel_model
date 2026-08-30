#ifndef PICO_SENSOR_H //這叫 include guard（標頭檔保護），作用是避免同一個 .h 被重複展開。
#define PICO_SENSOR_H //這個 header 在同一次編譯中只展開一次。

#define DEVICE_FILE_NAME "/dev/pico_sensor"
#define DEVICE_NAME "pico_sensor"

#ifdef __KERNEL__  //// 只有在 Kernel 環境編譯時才使用這段程式碼
#include <linux/types.h>
#include <linux/io.h>

/*address mapping*/
void __iomem *PERIBase;
void __iomem *GPIOBase;
void __iomem *RIOBase;

void __iomem *PADBase;
void __iomem *pad;

void __iomem *UART0_Base;

u32 uart0_txd = 14;
u32 uart0_rxd = 15;
u32 fn = 4;//仔細看，UART0_RX.TX是a4不是a2

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
/* UART */
#define UART_CR_OFFSET       0X030
#define UART_IBRD_OFFSET     0x024
#define UART_FBRD_OFFSET     0x028
#define UART_LCR_H_OFFSET    0x02C
#define UART_FR_OFFSET       0x018
#define UART_DR_OFFSET       0x000

// Atomic Access = N
// #define UART_XOR_OFFSET      0x1000
// #define UART_SET_OFFSET      0x2000
// #define UART_CLR_OFFSET      0x3000

#endif
#endif

//假設PIN = 20
//  如果你決定「整個計算都用十六進制表示」，那麼 PIN = 20（十進制）就要先表示成 0x14，再拿去算。

// 十進制、十六進制只是表示數值的方法，不是不同的數值。
// 20 = 0x14、8 = 0x08
// 所以：
// 20 × 0x08 = 0x14 × 0x08 = 0xA0

// PIN 20 = PIN 0x14，因為 20（十進制）= 0x14（十六進制）。