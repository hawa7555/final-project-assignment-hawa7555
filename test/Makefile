CC = gcc
CFLAGS = -Wall -I../driver

all: led_test audio_test audio_playback_test stt_test tts_test llm_test

led_test: led_test.c ../app/led_control.c
	$(CC) $(CFLAGS) -o led_test led_test.c ../app/led_control.c

audio_test: audio_test.c ../app/audio_capture.c
	$(CC) $(CFLAGS) -o audio_test audio_test.c ../app/audio_capture.c

audio_playback_test: audio_playback_test.c ../app/audio_capture.c ../app/audio_playback.c
	$(CC) $(CFLAGS) -o audio_playback_test audio_playback_test.c ../app/audio_capture.c ../app/audio_playback.c

stt_test: stt_test.c ../app/audio_capture.c ../app/stt_interface.c
	$(CC) $(CFLAGS) -o stt_test stt_test.c ../app/audio_capture.c ../app/stt_interface.c

tts_test: tts_test.c ../app/tts_interface.c ../app/audio_playback.c
	$(CC) $(CFLAGS) -o tts_test tts_test.c ../app/tts_interface.c ../app/audio_playback.c

llm_test: llm_test.c ../app/llm_interface.c
	$(CC) $(CFLAGS) -o llm_test llm_test.c ../app/llm_interface.c

clean:
	rm -f led_test audio_test audio_playback_test stt_test tts_test llm_test *.o *.wav

.PHONY: all clean
