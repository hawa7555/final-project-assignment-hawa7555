#ifndef STT_INTERFACE_H
#define STT_INTERFACE_H

#include <stddef.h>

/*
 * Transcribe audio file to text using Whisper
 */
int stt_transcribe(const char *audio_file, char *text_output, size_t max_len);

#endif
