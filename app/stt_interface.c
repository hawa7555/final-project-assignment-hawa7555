#include "stt_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

int stt_transcribe(const char *audio_file, char *text_output, size_t max_len)
{
    char cmd[1024];
    FILE *fp;
    char line[512];
    int found = 0;

    printf("Transcribing: %s\n", audio_file);

    // Capture stdout (+ stderr) from whisper-cli
    snprintf(cmd, sizeof(cmd),
             "/usr/bin/whisper-cli "
             "-m /root/models/whisper/ggml-tiny.en.bin "
             "-f '%s' 2>&1",
             audio_file);

    // Debug log
    printf("CMD: %s\n", cmd);

    fp = popen(cmd, "r");
    if (fp == NULL)
    {
        perror("popen");
        return -1;
    }

    text_output[0] = '\0';

    // Read ALL lines
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        // Raw whisper output:
        printf("WHISPER: %s", line);

        // Look for transcript lines:
        // [00:00:00.000 --> 00:00:05.000]
        char *br = strchr(line, ']');
        if (!br)
            continue;   // skip non-transcript lines

        char *text_start = br + 1;

        // Skip whitespace
        while (*text_start == ' ' || *text_start == '\t')
            text_start++;

        if (*text_start == '\0' || *text_start == '\n')
            continue;

        // Copy transcript text
        strncpy(text_output, text_start, max_len - 1);
        text_output[max_len - 1] = '\0';

        // Remove trailing newline
        char *newline = strchr(text_output, '\n');
        if (newline) *newline = '\0';

        found = 1;
        break;  // first transcript line
    }

    int status = pclose(fp);
    if (status == -1)
    {
        perror("pclose");
        return -1;
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
    {
        printf("whisper-cli exited with %d\n", WEXITSTATUS(status));
        if (!found)
            return -1;
    }

    if (!found)
    {
        printf("No transcription found\n");
        return -1;
    }

    printf("Transcribed: %s\n", text_output);
    return 0;
}

