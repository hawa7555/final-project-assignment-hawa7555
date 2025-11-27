#include "audio_playback.h"
#include <stdio.h>
#include <stdlib.h>

int audio_playback(const char *input_file)
{
    char cmd[512];
    int ret;
    
    printf("Playing audio: %s\n", input_file);
    
    // Use device name
    snprintf(cmd, sizeof(cmd), "aplay -D plughw:CARD=Headphones,DEV=0 %s 2>/dev/null", input_file);
    
    ret = system(cmd);
    
    if (ret != 0)
    {
        printf("Audio playback failed (exit code: %d)\n", ret);
        return -1;
    }
    
    printf("Playback complete\n");
    return 0;
}
