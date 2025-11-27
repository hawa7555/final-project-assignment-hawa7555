#include <stdio.h>
#include <unistd.h>
#include "../app/audio_capture.h"

int main(void)
{    
    printf("Recording 5 seconds... Speak now!\n");
    
    if (audio_capture("test_recording.wav", 5) < 0)
    {
        printf("Audio capture failed\n");
        return 1;
    }
    
    printf("\nRecording saved to test_recording.wav\n");
    printf("Play it with: aplay test_recording.wav\n");
    
    return 0;
}
