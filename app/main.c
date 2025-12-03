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
#include "sentence_queue.h"

#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>

#define AUDIO_INPUT  "input.wav"
#define AUDIO_OUTPUT "output.wav"
#define RECORD_DURATION 5

static volatile int running = 1;
static pthread_mutex_t led_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool pipeline_active = false;
static bool tts_done = false;
static bool playback_done = false;

static sentence_queue_t sentence_queue;
static audio_queue_t audio_queue;

static int last_start_state = 0;
static int last_cancel_state = 0;

// static audio_queue_t audio_queue;
static int sentence_counter = 0;
static pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
static int active_tts_threads = 0;
static pthread_mutex_t tts_count_mutex = PTHREAD_MUTEX_INITIALIZER;

void set_led_mutex(int status)
{
    pthread_mutex_lock(&led_mutex);
    led_set_status(status);
    pthread_mutex_unlock(&led_mutex);
}

void signal_handler(int sig)
{
    printf("\n\n Shutting down \n");
    running = 0;
    
    pthread_mutex_lock(&led_mutex);
    led_set_status(LED_STATUS_IDLE);
    led_cleanup();
    pthread_mutex_unlock(&led_mutex);
    
    exit(0);
}

// Read button states from driver 
int read_buttons(int *start_pressed, int *cancel_pressed)
{
    int fd;
    char buf[3];
    ssize_t bytes_read;
    
    fd = open("/dev/aitutor_led", O_RDONLY);
    if (fd < 0)
    {
        return -1;  // Device not available 
    }
    
    bytes_read = read(fd, buf, 3);
    close(fd);
    
    if (bytes_read != 3)
    {
        return -1;
    }
    
    *start_pressed = (buf[0] == '1');
    *cancel_pressed = (buf[1] == '1');
    
    return 0;
}

// Callback function for to push LLM sentence response to queue
void sentence_ready_callback(const char *sentence, void *user_data)
{
    // sentence += strspn(sentence, "\n\r\t");
    // while (*sentence == '\n') sentence++; 
    // printf("LLM streamed without \n: %s\n", sentence);
    // queue_push(&sentence_queue, sentence);

    char* ptr = (char*)sentence;
    
    while (*ptr == '\\' && *(ptr + 1) == 'n')
    {
        ptr += 2;
    }
    
    printf("LLM streamed without new: %s\n", ptr);
    fflush(stdout);
    
    queue_push(&sentence_queue, ptr);
}

// Playback Thread
void* playback_thread(void* arg)
{
    char pending_files[MAX_SENTENCES][256];
    int pending_count = 0;
    int next_to_play = 0;
    
    printf("Playback thread started\n");
    
    while (running)
    {
        // Wait for pipeline to become active 
        while (running && !pipeline_active)
        {
            usleep(100000);
        }
        
        if (!running) break;
        
        pending_count = 0;
        next_to_play = 0;
        
        // Play audio files
        while (running)
        {
            char filename[256];
            int found = -1;
            
            // Check if next expected file is already in pending list
            char expected[64];
            snprintf(expected, sizeof(expected), "output_%d.wav", next_to_play);
            
            for (int i = 0; i < pending_count; i++)
            {
                if (strstr(pending_files[i], expected))
                {
                    found = i;
                    break;
                }
            }
            
            if (found >= 0)
            {
                // Found the next file
                strcpy(filename, pending_files[found]);
                
                // Remove from pending list 
                for (int i = found; i < pending_count - 1; i++)
                {
                    strcpy(pending_files[i], pending_files[i + 1]);
                }
                pending_count--;
            } 
            else
            {
                // Need to get more files from queue 
                if (!audio_queue_pop(&audio_queue, filename, sizeof(filename)))
                {
                    // Queue done
                    if (pending_count == 0)
                    {
                        break;  // All done 
                    }

                    // pending files left
                    usleep(50000);
                    continue;
                }
                
                // Got a file, check with expected
                if (strstr(filename, expected))
                {
                    found = 0;         //found next one
                } else
                {
                    // Not right file in order, add to pending
                    strcpy(pending_files[pending_count++], filename);
                    continue;
                }
            }
            
            // Play the file 
            printf("Playing: %s\n", filename);
            
            if (audio_playback(filename) < 0)
            {
                printf("Audio playback failed: %s\n", filename);
            }
            
            unlink(filename);
            next_to_play++;
        }

        // Signal that playback is done 
        playback_done = true;
    }
    
    printf("Playback thread exiting\n");
    return NULL;
}

// TTS Thread
void* tts_thread(void* arg)
{
    char sentence[MAX_SENTENCE_LEN];
    char output_file[64];
    int my_sentence_num;
    int thread_id = *(int*)arg;
    
    printf("TTS thread %d started\n", thread_id);
    
    while (running)
    {
        // Wait for pipeline to become active 
        while (running && !pipeline_active)
        {
            usleep(100000);
        }
        
        if (!running) break;
        
        //Active TTS thread
        pthread_mutex_lock(&tts_count_mutex);
        active_tts_threads++;
        pthread_mutex_unlock(&tts_count_mutex);
        
        // Process sentences for this conversation 
        while (running && pipeline_active)
        {
            // Get next sentence from queue
            if (!queue_pop(&sentence_queue, sentence, sizeof(sentence)))
            {
                break;    //queue is empty
            }
            
            // Get unique sentence number
            pthread_mutex_lock(&counter_mutex);
            my_sentence_num = sentence_counter++;
            pthread_mutex_unlock(&counter_mutex);
            
            printf("TTS thread %d: Processing sentence %d: %s\n", thread_id, my_sentence_num, sentence);
            
            // create unique filename 
            snprintf(output_file, sizeof(output_file), "output_%d.wav", my_sentence_num);
            
            // Synthesize this sentence 
            if (tts_synthesize(sentence, output_file) < 0)
            {
                printf("TTS thread %d: synthesis failed for sentence %d\n", thread_id, my_sentence_num);
                continue;
            }
            
            // Add to queue for playback 
            audio_queue_push(&audio_queue, output_file);
        }
        
        // Not active TTS thread 
        pthread_mutex_lock(&tts_count_mutex);
        active_tts_threads--;
        
        // Last thread to finish marks audio queue as done 
        if (active_tts_threads == 0)
        {
            audio_queue_mark_done(&audio_queue);
            tts_done = true;
        }
        pthread_mutex_unlock(&tts_count_mutex);
    }
    
    printf("TTS thread %d exiting\n", thread_id);
    return NULL;
}

// Pipeline thread
void* pipeline_thread(void* arg)
{
    char user_text[512];
    
    printf("Pipeline thread started\n");
    
    while (running)
    {
        // Wait for signal to start processing 
        while (running && !pipeline_active)
        {
            usleep(100000);  // Sleep 100ms 
        }
        
        if (!running) break;
        
        printf("\n Processing Question \n");
        
        // Reset queues and counter for new conversation 
        queue_reset(&sentence_queue);
        audio_queue_reset(&audio_queue);
        
        pthread_mutex_lock(&counter_mutex);
        sentence_counter = 0;
        pthread_mutex_unlock(&counter_mutex);
        
        // 1. LISTENING - Capture audio 
        set_led_mutex(LED_STATUS_LISTENING);
        printf("Listening for %d seconds... Speak now!\n", RECORD_DURATION);
        
        if (audio_capture(AUDIO_INPUT, RECORD_DURATION) < 0)
        {
            printf("Audio capture failed\n");
            set_led_mutex(LED_STATUS_IDLE);
            pipeline_active = false;
            continue;
        }
        
        printf("\nProcessing speech...\n");
        
        // Transcribe speech to text 
        if (stt_transcribe(AUDIO_INPUT, user_text, sizeof(user_text)) < 0) 
        {
            printf("Speech transcription failed\n");
            set_led_mutex(LED_STATUS_IDLE);
            pipeline_active = false;
            continue;
        }
        
        // Check for blank audio 
        if (strstr(user_text, "[BLANK_AUDIO]") != NULL || strlen(user_text) < 3)
        {
            printf("No speech detected, skipping...\n");
            set_led_mutex(LED_STATUS_IDLE);
            pipeline_active = false;
            printf("\nPress Enter for next question...\n");
            continue;
        }

        printf("You asked: %s\n", user_text);

         // 2. THINKING & SPEAKING
        set_led_mutex(LED_STATUS_THINKING);
        printf("Thinking and speaking...\n");

        tts_done = false;
        playback_done = false;
        
        // Stream LLM query and add sentences in queue
        if (llm_query_stream(user_text, sentence_ready_callback, NULL) < 0)
        {
            printf("LLM stream query failed\n");
            queue_mark_done(&sentence_queue);
            set_led_mutex(LED_STATUS_IDLE);
            pipeline_active = false;
            continue;
        }
        
        // Mark queue as done
        queue_mark_done(&sentence_queue);
        
        // Speaking status
        set_led_mutex(LED_STATUS_SPEAKING);

        // Wait for TTS to finish synthesizing 
        while (!tts_done && running)
        {
            usleep(100000);
        }
        
        // Wait for playback to finish playing 
        while (!playback_done && running)
        {
            usleep(100000);
        }
        
        // Done, IDLE status
        set_led_mutex(LED_STATUS_IDLE);
        pipeline_active = false;
        printf("Press START button for next question (CANCEL to exit)...\n\n");
    }
    
    printf("Pipeline thread exiting\n");
    return NULL;
}

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    pthread_t pipeline_tid, tts_tid[2], playback_tid;
    int tts_thread_ids[2] = {1, 2};
    
    printf(" AI Tutor Starting \n");
    fprintf(stderr, "AI Tutor started - this is stderr\n");
    
    // Initialize queues 
    queue_init(&sentence_queue);
    audio_queue_init(&audio_queue);
    
    // Setup signal handler 
    signal(SIGINT, signal_handler);
    
    // Initialize LED control 
    if (led_init("/dev/aitutor_led") < 0) 
    {
        printf("Failed to initialize LED. Driver not loaded\n");
        return 1;
    }
    
    // Set initial state to IDLE 
    set_led_mutex(LED_STATUS_IDLE);
    printf("AI Tutor ready!\n");

    // Create playback thread 
    if (pthread_create(&playback_tid, NULL, playback_thread, NULL) != 0)
    {
        printf("Failed to create playback thread\n");
        led_cleanup();
        queue_cleanup(&sentence_queue);
        audio_queue_cleanup(&audio_queue);
        return 1;
    }
    
    // Create 2 TTS threads
    for (int i = 0; i < 2; i++)
    {
        if (pthread_create(&tts_tid[i], NULL, tts_thread, &tts_thread_ids[i]) != 0)
        {
            printf("Failed to create TTS thread %d\n", i+1);
            led_cleanup();
            queue_cleanup(&sentence_queue);
            audio_queue_cleanup(&audio_queue);
            return 1;
        }
    }
    
    // Create pipeline thread 
    if (pthread_create(&pipeline_tid, NULL, pipeline_thread, NULL) != 0)
    {
        printf("Failed to create pipeline thread\n");
        led_cleanup();
        queue_cleanup(&sentence_queue);
        return 1;
    }
    
    printf("Press START button to ask a question (CANCEL to exit)...\n\n");
    
    // Main thread, handle button input 
    while (running)
    {
        int start_now = 0, cancel_now = 0;
        
        // Read button states 
        if (read_buttons(&start_now, &cancel_now) == 0)
        {
            // Detect CANCEL button press
            if (cancel_now && !last_cancel_state)
            {
                printf("\n CANCEL button pressed - shutting down...\n");
                running = 0;
                break;
            }
            
            // Detect START button press
            if (start_now && !last_start_state)
            {
                // Check if pipeline is already busy 
                if (pipeline_active)
                {
                    printf("Please wait, still processing...\n");
                }
                else
                {
                    printf("\nSTART button pressed!\n");
                    fprintf(stderr, "Button pressed! Starting listening... (stderr)\n");
                    // Signal pipeline to start 
                    pipeline_active = true;
                }
            }
            
            // Update last states 
            last_start_state = start_now;
            last_cancel_state = cancel_now;
        }

        usleep(100000);  // sleep 100ms 
    }
    
    // Wait for threads to finish 
    printf("Waiting for threads to finish...\n");
    queue_mark_done(&sentence_queue);
    audio_queue_mark_done(&audio_queue);

    pthread_join(pipeline_tid, NULL);
    pthread_join(tts_tid[0], NULL);
    pthread_join(tts_tid[1], NULL);
    pthread_join(playback_tid, NULL);
    
    // Cleanup 
    queue_cleanup(&sentence_queue);
    audio_queue_cleanup(&audio_queue);
    
    printf("Goodbye!\n");
    return 0;
}
