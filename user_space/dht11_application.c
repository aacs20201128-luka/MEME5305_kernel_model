#include <stdio.h>      // printf(), perror() 等標準輸入輸出函式
#include <stdlib.h>     // exit(), malloc(), free() 等函式
#include <fcntl.h>      // open()、O_RDONLY、O_WRONLY、O_RDWR
#include <unistd.h>     // read(), write(), close() 等系統呼叫 、pause()
#include <sys/ioctl.h>  // ioctl() 裝置控制函式
#include <string.h>     // strlen(), strcpy(), strcmp(), memset() 等字串處理函式
#include <signal.h>     // userspace signal 定義與處理函式，如 signal()、SIGUSR1

#include "../dht11_sensor.h"

int main(void){
    int file_desc;
    int len;
    unsigned char message[4];

    file_desc = open(DEVICE_FILE_NAME, O_RDONLY);

    len = read(file_desc, message, sizeof(message));

    if (len < 0) {
        perror("read");
        close(file_desc);
        return 1;
    }

    printf("Humidity: %u.%u %%\n", message[0], message[1]);
    printf("Temperature: %u.%u C\n", message[2], message[3]);

    close(file_desc);
    return 0;
}


// DHT11 有一個值得注意的地方：它不適合被非常高頻地 read()。
//  比較好的架構通常是 每次 user-space read() 時觸發一次量測，
//  或者 kernel 定期更新資料、read() 只拿最新值。

// perror("錯誤前綴文字") // 前綴內容可自訂，真正的系統錯誤訊息會根據 errno 自動補上

// unsigned char message[4] 代表 User Space 準備 4 Bytes 空間。
// read(file_desc, message, sizeof(message)) 會告訴 Kernel 最多讀 4 Bytes，所以 length = 4。
// Kernel 的 min(length, (size_t)4) 會得到 4，最後 copy_to_user(buffer, data, 4) 
//  就把 4 Bytes 資料複製到 message[4]。

// copy_to_user() // 看「位址 + 要複製幾個 Bytes」，不是看兩邊型別是否一樣
// 但要注意：大小一樣 ≠ 資料意義一定一樣。
//  型別不同時，還要考慮資料排列、endianness、struct padding 等問題。