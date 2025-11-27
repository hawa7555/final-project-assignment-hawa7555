#include <stdio.h>
#include "../app/audio_capture.h"
#include "../app/audio_playback.h"

int main(void)
{
    printf("Audio Record & Playback Test ===\n\n");
    
    printf("Recording 5 seconds... Speak now!\n");
    if (audio_capture("test_recording.wav", 5) < 0) {
        return 1;
    }
    
    printf("\nPlaying back recording...\n");
    if (audio_playback("test_recording.wav") < 0) {
        return 1;
    }
    
    printf("\n=== Test Complete ===\n");
    return 0;
}
