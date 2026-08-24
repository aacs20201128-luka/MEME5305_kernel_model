TARGET=application

all: $(TARGET) #all 就是只下make或make all

CC = gcc

CFLAGS = -g -Wall -ansi

$(TARGET) : $(TARGET).c
	$(CC) $(TARGET).c -o $(TARGET)

clean:
	rm -f $(TARGET)

#不會去編譯 application.c。
#-ansi 把編譯標準卡在 C90，所以你大量使用的 // 註解直接炸掉了
mmap: mmap.c
	$(CC) -g -Wall mmap.c -o mmap
clean_mmap :
	rm -f mmap