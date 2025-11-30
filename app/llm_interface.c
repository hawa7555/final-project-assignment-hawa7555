#include "llm_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int llm_query(const char *user_query, char *response, size_t max_len)
{
    char cmd[2048];
    char json_file[] = "/tmp/llm_request.json";
    char response_file[] = "/tmp/llm_response.json";
    FILE *fp;
    char line[1024];
    int found = 0;
    
    printf("Querying LLM: %s\n", user_query);
    
    // Create JSON request
    fp = fopen(json_file, "w");
    if (!fp)
    {
        printf("Failed to create request file\n");
        return -1;
    }
    
    fprintf(fp, "{\n");
    fprintf(fp, "  \"messages\": [\n");
    fprintf(fp, "    {\"role\": \"system\", \"content\": \"You are a helpful tutor. Give ONLY a direct answer in 1-2 short sentences. Do NOT use any special tags, markers, or format indicators. Just plain text answers.\"},\n");
    fprintf(fp, "    {\"role\": \"user\", \"content\": \"%s\"}\n", user_query);
    fprintf(fp, "  ],\n");
    fprintf(fp, "  \"max_tokens\": 100,\n");
    fprintf(fp, "  \"temperature\": 0.5\n");
    fprintf(fp, "}\n");
    fclose(fp);
    
    // Send request to llama-server
    snprintf(cmd, sizeof(cmd), "curl -s http://localhost:8080/v1/chat/completions " "-H 'Content-Type: application/json' " "-d @%s > %s", json_file, response_file);
    
    if (system(cmd) != 0)
    {
        printf("Failed to query LLM server\n");
        return -1;
    }
    
    // Parse response - extract content field
    snprintf(cmd, sizeof(cmd), "grep -o '\"content\":\"[^\"]*\"' %s | head -1 | sed 's/\"content\":\"//' | sed 's/\"$//'", response_file);
    
    fp = popen(cmd, "r");
    if (!fp)
    {
        printf("Failed to parse response\n");
        return -1;
    }
    
    // Read extracted content
    response[0] = '\0';
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        strncat(response, line, max_len - strlen(response) - 1);
        found = 1;
    }
    pclose(fp);
    
    // Remove trailing newline
    char *newline = strchr(response, '\n');
    if (newline) *newline = '\0';
    
    // Cleanup
    remove(json_file);
    remove(response_file);
    
    if (!found || strlen(response) == 0)
    {
        printf("No response from LLM\n");
        return -1;
    }
    
    printf("LLM response: %s\n", response);
    return 0;
}

/* Stream LLM response sentence by sentence 
 * Calls callback function for each complete sentence */
int llm_query_stream(const char *user_query, void (*sentence_callback)(const char *sentence, void *user_data), void *user_data)
{
    char cmd[2048];
    char json_file[] = "/tmp/llm_request.json";
    FILE *fp, *stream;

    char line[1024];
    char current_sentence[1024] = "";
    char sentence_buffer[1024] = "";  // Buffer to accumulate short sentences
    int buffer_len = 0;
    const int MIN_SENTENCE_LENGTH = 120;  // Minimum chars before sending to TTS
    
    printf("Streaming LLM query: %s\n", user_query);
    
    // Create JSON request
    fp = fopen(json_file, "w");
    if (!fp)
    {
        printf("Failed to create request file\n");
        return -1;
    }
    
    fprintf(fp, "{\n");
    fprintf(fp, "  \"messages\": [\n");
    fprintf(fp, "    {\"role\": \"system\", \"content\": \"You are a helpful tutor. Give ONLY a direct answer in 1-2 short sentences. Do NOT use any special tags, markers, or format indicators. Just plain text answers.\"},\n");
    fprintf(fp, "    {\"role\": \"user\", \"content\": \"%s\"}\n", user_query);
    fprintf(fp, "  ],\n");
    fprintf(fp, "  \"max_tokens\": 100,\n");
    fprintf(fp, "  \"temperature\": 0.5,\n");
    fprintf(fp, "  \"stream\": true\n");
    fprintf(fp, "}\n");
    fclose(fp);
    
    /* Stream request to llama-server */
    snprintf(cmd, sizeof(cmd), "curl -N -s http://localhost:8080/v1/chat/completions " "-H 'Content-Type: application/json' " "-d @%s 2>/dev/null", json_file);
    
    stream = popen(cmd, "r");
    if (!stream)
    {
        printf("Failed to stream from LLM server\n");
        remove(json_file);
        return -1;
    }
    
    // Process streaming chunks
    while (fgets(line, sizeof(line), stream) != NULL)
    {
        // Skip empty lines and [DONE]
        if (line[0] == '\n' || strstr(line, "[DONE]")) {
            continue;
        }
        
        // Parse data: {...} lines
        if (strncmp(line, "data: ", 6) == 0)
        {
            char *json_start = line + 6;
            
            // Extract content field
            char *content_start = strstr(json_start, "\"content\":\"");
            if (content_start)
            {
                content_start += 11;  // Skip "content":"
                char *content_end = strchr(content_start, '"');
                
                if ( (content_end != NULL) && (content_end > content_start) )
                {
                    int len = content_end - content_start;
                    char token[256];
                    
                    if (len > 0 && len < sizeof(token))
                    {
                        strncpy(token, content_start, len);
                        token[len] = '\0';
                        
                        // Append to current sentence
                        strncat(current_sentence, token, sizeof(current_sentence) - strlen(current_sentence) - 1);
                        
                        // Check for sentence boundary
                        if (strchr(token, '.') || strchr(token, '!') || strchr(token, '?'))
                        {
                            // Found complete sentence - add to buffer
                            if (strlen(current_sentence) > 0)
                            {
                                strncat(sentence_buffer, current_sentence,  sizeof(sentence_buffer) - strlen(sentence_buffer) - 1);
                                buffer_len = strlen(sentence_buffer);
                                current_sentence[0] = '\0';
                                
                                // Send buffer if it's long enough
                                if (buffer_len >= MIN_SENTENCE_LENGTH)
                                {
                                    sentence_callback(sentence_buffer, user_data);
                                    sentence_buffer[0] = '\0';
                                    buffer_len = 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Send any remaining content in current sentence
    if (strlen(current_sentence) > 0)
    {
        strncat(sentence_buffer, current_sentence, sizeof(sentence_buffer) - strlen(sentence_buffer) - 1);
    }
    
    // Send any remaining buffered content
    if (strlen(sentence_buffer) > 0) {
        sentence_callback(sentence_buffer, user_data);
    }
    
    pclose(stream);
    remove(json_file);
    
    printf("LLM streaming complete\n");
    return 0;
}

//Resources: AI help was used to correctly parse the output in llm_query_stream() function and for edge cases
