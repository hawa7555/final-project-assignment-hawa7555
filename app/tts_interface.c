#include "tts_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tts_synthesize(const char *text, const char *output_file)
{
    char cmd[2048];
    char escaped_text[1024];
    int i, j;
    
    printf("Synthesizing: %s\n", text);
    
    // Escape single quotes
    j = 0;
    for (i = 0; text[i] && j < sizeof(escaped_text) - 4; i++)
    {
        if (text[i] == '\'')
        {
            escaped_text[j++] = '\'';
            escaped_text[j++] = '\\';
            escaped_text[j++] = '\'';
            escaped_text[j++] = '\'';
        }
        else
        {
            escaped_text[j++] = text[i];
        }
    }
    escaped_text[j] = '\0';
    
    // Use single quotes
    snprintf(cmd, sizeof(cmd), "echo '%s' | /home/harsh/piper/piper --model /home/harsh/piper/en_US-lessac-medium.onnx --output_file %s 2>/dev/null", escaped_text, output_file);
    
    printf("%s\n", output_file);
    
    int ret = system(cmd);
    if (ret != 0)
    {
        printf("TTS synthesis failed (exit code: %d)\n", ret);
        return -1;
    }
    
    printf("Synthesis complete: %s\n", output_file);
    return 0;
}
