#include "stt_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int stt_transcribe(const char *audio_file, char *text_output, size_t max_len)
{
    char cmd[1024];
    FILE *fp;
    char line[512];
    int found = 0;
    
    printf("Transcribing: %s\n", audio_file);
    
    // Set library path and run whisper-cli
    snprintf(cmd, sizeof(cmd), "export LD_LIBRARY_PATH=/home/harsh/aesd/whisper.cpp/build/src:/home/harsh/aesd/whisper.cpp/build/ggml/src && "
             "/home/harsh/aesd/whisper.cpp/build/bin/whisper-cli "
             "-m /home/harsh/aesd/whisper.cpp/models/ggml-tiny.en.bin "
             "-f %s 2>&1 | grep '\\[00:' | head -1",
             audio_file);
    
    fp = popen(cmd, "r");
    if (fp == NULL)
    {
        printf("Failed to run whisper\n");
        return -1;
    }
    
    // Read output line
    if (fgets(line, sizeof(line), fp) != NULL)
    {
        // Parse: [00:00:00.000 --> 00:00:05.000]
        char *text_start = strstr(line, "]");
        if (text_start)
        {
            text_start += 1;  // Skip ']'
            
            // Skip whitespace
            while (*text_start == ' ' || *text_start == '\t')
            {
                text_start++;
            }
            
            // Copy to output, removing newline
            strncpy(text_output, text_start, max_len - 1);
            text_output[max_len - 1] = '\0';
            
            // Remove trailing newline
            char *newline = strchr(text_output, '\n');
            if (newline) *newline = '\0';
            
            found = 1;
        }
    }
    
    pclose(fp);
    
    if (!found)
    {
        printf("No transcription found\n");
        return -1;
    }
    
    printf("Transcribed: %s\n", text_output);
    return 0;
}

//Resources: https://www.gnu.org/software/libc/manual/html_node/Pipe-to-a-Subprocess.html
