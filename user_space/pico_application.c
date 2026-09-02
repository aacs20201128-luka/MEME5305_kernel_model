#include <stdio.h>      // printf(), perror() 等標準輸入輸出函式
#include <stdlib.h>     // exit(), malloc(), free() 等函式
#include <fcntl.h>      // open()、O_RDONLY、O_WRONLY、O_RDWR
#include <unistd.h>     // read(), write(), close() 等系統呼叫 、pause()
#include <sys/ioctl.h>  // ioctl() 裝置控制函式
#include <string.h>     // strlen(), strcpy(), strcmp(), memset() 等字串處理函式
#include <signal.h>     // userspace signal 定義與處理函式，如 signal()、SIGUSR1

#include "../pico_sensor.h"
#include "../mariadb/mariadb.h"

int main(void){
    int file_desc;
    int len;
    unsigned char message[80]; // 0 ~ 255

    file_desc = open(DEVICE_FILE_NAME, O_RDONLY);

    len = read(file_desc, message, sizeof(message) - 1);

    if (len < 0) {
        perror("read");
        close(file_desc);
        return 1;
    }

    message[len] = '\0';
    printf("%s",message);
    // for (int i = 0; i < len; i++) {
    //     printf("%c", message[i]);
    // }

    if(connection_mariadb()){
        fprintf(stderr, "connection_mariadb failed\n");
        return 1;
    } 
    if(insert_bh1750_mariadb(message + 4)){ //跳過LUX=
        fprintf(stderr, "insert_bh1750_mariadb failed\n");
    }
    if(select_latest_bh1750_data()){
        fprintf(stderr, "select_latest_bh1750_data failed\n");
    }
    if(disconnection_mariadb()){
        fprintf(stderr, "disconnection_mariadb failed\n");
    }


    close(file_desc);
    return 0;
}

//  如果 read() 讀回來的是文字資料，而且你有預留 1 byte 空間，就可以自己補 \0：
//      len = read(fd, message, sizeof(message) - 1);
//      message[len] = '\0';
//  這樣 message 就是合法 C 字串，可以安全用：printf("%s", message);

// sizeof為甚麼要-1?    因為要保留最後 1 byte 給 \0。