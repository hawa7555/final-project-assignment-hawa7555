#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

/* 
 * Capture audio from microphone and save to WAV file
 */
int audio_capture(const char *output_file, int duration_sec);

#endif
