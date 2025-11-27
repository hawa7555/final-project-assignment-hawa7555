#ifndef SENTENCE_QUEUE_H
#define SENTENCE_QUEUE_H

#include <pthread.h>
#include <stdbool.h>

#define MAX_SENTENCES 10
#define MAX_SENTENCE_LEN 512

typedef struct {
    char sentences[MAX_SENTENCES][MAX_SENTENCE_LEN];
    int head;
    int tail;
    int count;
    bool done;  // Signal that LLM is complete
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} sentence_queue_t;

// Initialize queue 
void queue_init(sentence_queue_t *q);

// Add sentence to queue
void queue_push(sentence_queue_t *q, const char *sentence);

// Get sentence from queue
bool queue_pop(sentence_queue_t *q, char *sentence, size_t max_len);

// Signal that no more sentences will be added
void queue_mark_done(sentence_queue_t *q);

// Cleanup queue 
void queue_cleanup(sentence_queue_t *q);

//reset queue
void queue_reset(sentence_queue_t *q);

//Similar structure for audio queue

// Audio file queue for playback 
typedef struct {
    char filenames[MAX_SENTENCES][256];
    int head;
    int tail;
    int count;
    bool done;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} audio_queue_t;

// Audio queue functions 
void audio_queue_init(audio_queue_t *q);
void audio_queue_push(audio_queue_t *q, const char *filename);
bool audio_queue_pop(audio_queue_t *q, char *filename, size_t max_len);
void audio_queue_mark_done(audio_queue_t *q);
void audio_queue_reset(audio_queue_t *q);
void audio_queue_cleanup(audio_queue_t *q);

#endif
