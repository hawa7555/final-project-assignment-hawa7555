#ifndef LLM_INTERFACE_H
#define LLM_INTERFACE_H
#include <stddef.h>

// Query the LLM server with a user question
int llm_query(const char *user_query, char *response, size_t max_len);

// Stream LLM response with callback for each sentence
int llm_query_stream(const char *user_query, 
                     void (*sentence_callback)(const char *sentence, void *user_data),
                     void *user_data);

#endif
