CC = gcc
CFLAGS = -Wall -I../driver

all: led_test

led_test: led_test.c ../app/led_control.c
	$(CC) $(CFLAGS) -o led_test led_test.c ../app/led_control.c
	
clean:
	rm -f led_test *.o *.wav
	
.PHONY: all clean
