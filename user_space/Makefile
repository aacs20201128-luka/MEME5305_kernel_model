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
mmap_c :
	rm -f mmap

#user_ioctl.c
ioctl: user_ioctl.c
	$(CC) -g -Wall user_ioctl.c -o ioctl
ioctl_c :
	rm -f ioctl

MARIADB_FLAGS = $(shell mariadb_config --cflags --libs)
DB_DIR = ../mariadb

#dht11_application.c
dht11: dht11_application.c $(DB_DIR)/mariadb.c $(DB_DIR)/mariadb.h
	$(CC) -g -Wall \
	dht11_application.c \
	$(DB_DIR)/mariadb.c \
	-I$(DB_DIR) \
	-o dht11_application \
	$(MARIADB_FLAGS)
dht11_c :
	rm -f dht11_application

#pico_application.c
pico: pico_application.c
	$(CC) -g -Wall pico_application.c -o pico_application
pico_c :
	rm -f pico_application