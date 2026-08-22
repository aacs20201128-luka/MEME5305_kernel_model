#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include "request.h"

// .ko 不一定要自己寫 module_init() 和 module_exit()。
// init/exit 是「載入/卸載時要做什麼」，不是 .ko 存在的必要條件。
// insmod 載入的是 .ko；有沒有自訂 init/exit，只影響載入與卸載時有沒有額外工作，不決定能不能 insmod。
void request_test(void){
    printk(KERN_EMERG "request()\n");
}

//「你現在有沒有被載進來，沒有的話幫我載」，但要在/lib/modules/$(uname -r)/extra/內
//request_module("request"); 
//功能，是在執行期間需要時把 request.ko 載入 Kernel；
//  載入後，其他 .ko 可以使用它所註冊或公開的功能。
//request_test(); //註解掉，測試讓編譯時不要形成符號相依

EXPORT_SYMBOL(request_test);//要編成獨立的 request.ko
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("request module");