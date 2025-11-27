#include <stdio.h>
#include "../app/tts_interface.h"
#include "../app/audio_playback.h"

int main(void)
{
    const char *test_text = "A kernel module is a piece of code that extends the Linux kernel without rebooting.";
    
    printf("TTS Interface Test\n\n");
    
    printf("Synthesizing text to speech...\n");
    if (tts_synthesize(test_text, "test_tts.wav") < 0)
    {
        return 1;
    }
    
    printf("\nPlaying synthesized speech...\n");
    if (audio_playback("test_tts.wav") < 0)
    {
        return 1;
    }
    
    printf("\nTest Complete\n");
    return 0;
}
