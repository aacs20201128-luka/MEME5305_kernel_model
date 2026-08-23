TARGET=application

all: $(TARGET)

CC = gcc

CFLAGS = -g -Wall -ansi

$(TARGET) : $(TARGET).c
	$(CC) $(TARGET).c -o $(TARGET)

clean:
	rm -f $(TARGET)