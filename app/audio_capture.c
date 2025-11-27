#include "audio_capture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int audio_capture(const char *output_file, int duration_sec)
{
    char cmd[512];
    int ret;
    
    printf("Recording %d seconds to %s\n", duration_sec, output_file);
    
    // device name instead of card number
    snprintf(cmd, sizeof(cmd), "arecord -D plughw:CARD=AK2,DEV=0 -f S16_LE -r 16000 -c 1 -d %d %s 2>/dev/null", duration_sec, output_file);
    
    ret = system(cmd);
    
    if (ret != 0)
    {
        printf("Audio capture failed (exit code: %d)\n", ret);
        return -1;
    }
    
    printf("Recording complete: %s\n", output_file);
    return 0;
}
