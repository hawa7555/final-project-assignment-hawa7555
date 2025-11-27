#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "led_control.h"
#include "audio_capture.h"
#include "audio_playback.h"
#include "stt_interface.h"
#include "llm_interface.h"
#include "tts_interface.h"

#define AUDIO_INPUT  "input.wav"
#define AUDIO_OUTPUT "output.wav"
#define RECORD_DURATION 5

static volatile int running = 1;

void signal_handler(int sig)
{
    printf("\n\n Shutting down\n");
    led_set_status(LED_STATUS_IDLE);
    led_cleanup();
    exit(0);
}

int main(int argc, char *argv[])
{
    char user_text[512];
    char llm_response[1024];
    
    printf("AI Tutor Starting \n");
    
    // Setup signal handler
    signal(SIGINT, signal_handler);
    
    // Initialize LED control
    if (led_init("/dev/aitutor_led") < 0)
    {
        printf("Failed to initialize LED. Driver not loaded\n");
        return 1;
    }
    
    // Set initial state to IDLE
    led_set_status(LED_STATUS_IDLE);
    printf("AI Tutor ready!\n");
    printf("Press Enter to ask a question (Ctrl+C to exit)...\n\n");
    
    // Main conversation loop 
    while (running)
    {
        getchar();  // Wait for Enter
        
        if (!running) break;
        
        printf("\n New Question \n");
        
        // 1. LISTENING - Capture audio 
        led_set_status(LED_STATUS_LISTENING);  // Red LED
        printf("Listening for %d seconds... Speak now!\n", RECORD_DURATION);
        
        if (audio_capture(AUDIO_INPUT, RECORD_DURATION) < 0)
        {
            printf("Audio capture failed\n");
            led_set_status(LED_STATUS_IDLE);
            continue;
        }

        printf("Processing speech...\n");
        
        // Transcribe speech to text 
        if (stt_transcribe(AUDIO_INPUT, user_text, sizeof(user_text)) < 0)
        {
            printf("Speech transcription failed\n");
            led_set_status(LED_STATUS_IDLE);
            continue;
        }

        printf("You asked: %s\n", user_text);
        
        // 2. THINKING - LLM only
        led_set_status(LED_STATUS_THINKING);  // Green LED
        printf("Thinking...\n");
        
        // Query LLM 
        if (llm_query(user_text, llm_response, sizeof(llm_response)) < 0)
        {
            printf("LLM query failed\n");
            led_set_status(LED_STATUS_IDLE);
            continue;
        }

        printf("AI answer: %s\n", llm_response);
        
        // 3. SPEAKING - Synthesize and play
        led_set_status(LED_STATUS_SPEAKING);  // Red LED
        printf("Synthesizing response...\n");
        
        // Synthesize response to speech 
        if (tts_synthesize(llm_response, AUDIO_OUTPUT) < 0)
        {
            printf("Speech synthesis failed\n");
            led_set_status(LED_STATUS_IDLE);
            continue;
        }
        
        printf("Speaking...\n");
        
        if (audio_playback(AUDIO_OUTPUT) < 0)
        {
            printf("Audio playback failed\n");
            led_set_status(LED_STATUS_IDLE);
            continue;
        }
        
        // Back to IDLE 
        led_set_status(LED_STATUS_IDLE);
        printf("\nPress Enter for next question (Ctrl+C to exit)...\n");
    }
    
    // Cleanup 
    led_set_status(LED_STATUS_IDLE);
    led_cleanup();
    printf("\nAI Tutor Stopped \n");
    
    return 0;
}
