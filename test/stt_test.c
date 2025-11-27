#include <stdio.h>
#include "../app/audio_capture.h"
#include "../app/stt_interface.h"

int main(void)
{
    char transcribed_text[512];
    
    printf("Recording 5 seconds...Speak now!\n");
    if (audio_capture("test_stt.wav", 5) < 0)
    {
        return 1;
    }
    
    printf("\nTranscribing...\n");

    if (stt_transcribe("test_stt.wav", transcribed_text, sizeof(transcribed_text)) < 0)
    {
        return 1;
    }
    
    printf("Transcribed: %s\n", transcribed_text);
    
    return 0;
}
