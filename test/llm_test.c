#include <stdio.h>
#include "../app/llm_interface.h"

int main(void)
{
    char response[1024];
    const char *question = "What is a kernel module?";
    
    printf("LLM Interface Test\n\n");
    
    printf("Question: %s\n\n", question);
    
    if (llm_query(question, response, sizeof(response)) < 0)
    {
        printf("LLM query failed\n");
        return 1;
    }
    
    printf("Answer: %s\n", response);
    
    return 0;
}
