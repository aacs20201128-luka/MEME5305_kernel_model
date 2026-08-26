#ifndef LED_CHARDEV_H
#define LED_CHARDEV_H

#include <linux/ioctl.h> // ioctl 指令編碼相關巨集
// _IO()      // 不傳資料，只送指令
// _IOR()     // User Space 從 Kernel 讀資料
// _IOW()     // User Space 寫資料給 Kernel
// _IOWR()    // User Space 與 Kernel 雙向傳資料
// 方向是以 User Space 的角度命名：
//  R = Read  → User 從 Kernel 讀
//  W = Write → User 寫給 Kernel

#define LED_MAGIC 'L'
#define IOCTL_SET_LED_ON _IO(LED_MAGIC,10)
#define IOCTL_SET_LED_OFF _IO(LED_MAGIC,11)
#define IOCTL_TOGGLE_NTH_LED _IOR(LED_MAGIC,12,int)
#define DEVICE_FILE_NAME "/dev/kernel_ioctl"
#endif

// 666 → Type / Magic Magic 欄位只有 8-bit → 可表示 0 ~ 255
// 10  → Number
// int → Size，Kernel 會用 sizeof(int) 算大小

// ioctl number 是一個 32-bit 編碼值，主要用來避免不同驅動程式的指令號碼互相衝突。
//  Type / Magic（8-bit）：識別是哪個驅動程式，例如 'L'；也可以直接使用數字。
//  Number（8-bit）：指令流水號，用來區分不同的 ioctl 指令。
//  Size（14-bit）：要傳遞的資料大小。
//  Direction（2-bit）：資料傳遞方向，包含 None、Read、Write、Read/Write。

// 同一個 Driver 的 ioctl 通常使用相同 Magic / Type。
// 不同指令使用不同 Command Number。
// Magic 最好避免和其他 Driver 重複，方便維護並避免指令編碼衝突。
// User Space 與 Kernel Driver 要共用同一份 ioctl header，確保指令編碼一致。

// LED_MAGIC 可以把它想成這組 ioctl 指令的「代號」。
//  它主要在 #define ioctl 指令時使用，用來組成完整的 ioctl 編號。
//  User Space 和 Kernel Space 通常不會直接操作 LED_MAGIC，而是直接使用像 IOCTL_SET_LED_ON 
//    這種已經組好的指令。