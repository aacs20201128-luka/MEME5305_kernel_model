#include <stdio.h>      // printf(), perror() 等標準輸入輸出函式
#include <stdlib.h>     // exit(), malloc(), free() 等函式
#include <fcntl.h>      // open()、O_RDONLY、O_WRONLY、O_RDWR
#include <unistd.h>     // read(), write(), close() 等系統呼叫 、pause()
#include <sys/ioctl.h>  // ioctl() 裝置控制函式
#include <string.h>     // strlen(), strcpy(), strcmp(), memset() 等字串處理函式
#include <signal.h>     // userspace signal 定義與處理函式，如 signal()、SIGUSR1
// RD = Read
// WR = Write

void ouch(int sig)//signal handler
{
  printf("ouch!---I got signal %d\n",sig);
  //signal(SIGUSR1,SIG_DFL);  //SIGUSR1 的預設行為通常是 終止 process。
}


static char *dev = "/dev/xxx" , *info , message[100];

int main(int argc, char *argv[]){
    int user_pid;
    user_pid=getpid();
    printf("user space pid:%d\n",user_pid);
    signal(SIGUSR1,ouch);
    pause();//前景執行：終端機會像「卡住」一樣，直到它收到 signal 或被結束。
    //pause()收到SIGUSR1→執行ouch()→ouch()結束→馬上執行pause()的下一行，不需要再echo一次。

    if(argc != 3) return 1;
    dev = argv[1];
    info = argv[2];
    int file_desc;
    int len;

    file_desc = open(dev, O_RDWR);
    //先write後read避免chardev.c內的read讀空，除非在open事先賦值。
    //strlen(info)+1那就會連'\0'一起傳不是錯，但前提是你的kernel driver有把這個byte算進len
    len = write(file_desc , info , strlen(info));
    printf("application_write() len:%d\n", len);
    if(len < 0){
        perror("Write fail:");//perror()會自動把「系統錯誤原因」印出來。
    }
    len = read(file_desc , message , sizeof(message) - 1);//最多讀99bytes
    printf("application_read() message:%s, len:%d\n",message, len);
    close(file_desc);

    return 0;
}