#include <stdio.h>      // printf(), perror() 等標準輸入輸出函式
#include <stdlib.h>     // exit(), malloc(), free() 等函式
#include <fcntl.h>      // open()、O_RDONLY、O_WRONLY、O_RDWR
#include <unistd.h>     // read(), write(), close() 等系統呼叫
#include <sys/ioctl.h>  // ioctl() 裝置控制函式
#include <string.h>     // strlen(), strcpy(), strcmp(), memset() 等字串處理函式
// RD = Read
// WR = Write

static char *dev = "/dev/xxx" , *info , message[100];

int main(int argc, char *argv[]){
    if(argc != 3) return 1;
    dev = argv[1];
    info = argv[2];
    int file_desc;
    int len;

    file_desc = open(dev, O_RDWR);
    //先write後read避免chardev.c內的read讀空，除非在open事先賦值。
    //strlen(info)+1那就會連'\0'一起傳不是錯，但前提是你的kernel driver有把這個byte算進len
    len = write(file_desc , info , strlen(info));
    printf("write len:%d\n", len);
    if(len < 0){
        perror("Write fail:");//perror()會自動把「系統錯誤原因」印出來。
    }
    len = read(file_desc , message , sizeof(message) - 1);//最多讀99bytes
    printf("message:%s, len:%d\n",message, len);
    close(file_desc);
    return 0;
}