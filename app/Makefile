CC = gcc
CFLAGS = -Wall -I../driver
LDFLAGS = 

TARGET = aitutor

SRCS = main.c led_control.c audio_capture.c audio_playback.c stt_interface.c llm_interface.c tts_interface.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS) *.wav

.PHONY: all clean
