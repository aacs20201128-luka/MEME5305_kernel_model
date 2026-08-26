#include <stdio.h>          // printf()、perror() 等輸入輸出函式
#include <stdlib.h>         // exit()、malloc() ;exit(任何數值)都會直接結束整個程式。
#include <fcntl.h>          // open() 與檔案開啟旗標
#include <unistd.h>         // close()、read()、write() 等系統呼叫
#include <sys/ioctl.h>      // User Space 的 ioctl() 函式
#include "../kernel_ioctl.h"   // 自訂 ioctl 指令與共用定義

int main(int argc, char *argv[])
{
	int fd, retval, ret;

	fd = open(DEVICE_FILE_NAME, O_RDWR);//read + write 都可以
	printf("DEVICE_FILE_NAME:%s, fd:%d\n",DEVICE_FILE_NAME, fd);

  //test_ioctl.c
	for(int i = 0; i < 2; i++) {
		retval = ioctl(fd, IOCTL_SET_LED_ON, 0);

		sleep(1);
		
		retval = ioctl(fd, IOCTL_SET_LED_OFF, 0 );

		sleep(1);
		
		retval = ioctl(fd, IOCTL_TOGGLE_NTH_LED, 1);

		sleep(1);
	}

  //test_write.c
  for(int i = 0; i < 2; i++) {
		ret = write(fd, "1", 1);
	
		sleep(1);

		write(fd, "0", 1);
		sleep(1);
	}


	close(fd);

	return 0;
}

// ioctl(fd, 指令, 額外資料)
//  fd       = 裝置檔的 file descriptor
//  request  = ioctl 指令編號
//  ...      = 可選的第三個參數，通常是數值或資料位址
//  回傳值   = 成功通常回傳 0，失敗回傳 -1