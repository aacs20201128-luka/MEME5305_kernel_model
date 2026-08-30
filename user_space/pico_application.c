#include <stdio.h>      // printf(), perror() 等標準輸入輸出函式
#include <stdlib.h>     // exit(), malloc(), free() 等函式
#include <fcntl.h>      // open()、O_RDONLY、O_WRONLY、O_RDWR
#include <unistd.h>     // read(), write(), close() 等系統呼叫 、pause()
#include <sys/ioctl.h>  // ioctl() 裝置控制函式
#include <string.h>     // strlen(), strcpy(), strcmp(), memset() 等字串處理函式
#include <signal.h>     // userspace signal 定義與處理函式，如 signal()、SIGUSR1

#include "../pico_sensor.h"

int main(void){
    int file_desc;
    int len;
    unsigned char message[80]; // 0 ~ 255

    file_desc = open(DEVICE_FILE_NAME, O_RDONLY);

    len = read(file_desc, message, sizeof(message));

    if (len < 0) {
        perror("read");
        close(file_desc);
        return 1;
    }

    for (int i = 0; i < len; i++) {
        printf("%c", message[i]);
    }
    // printf("\n");

    close(file_desc);
    return 0;
}