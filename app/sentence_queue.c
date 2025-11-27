#include "sentence_queue.h"
#include <string.h>

void queue_init(sentence_queue_t *q)
{
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->done = false;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

void queue_push(sentence_queue_t *q, const char *sentence)
{
    pthread_mutex_lock(&q->mutex);
    
    // Wait if queue is full 
    while (q->count >= MAX_SENTENCES)
    {
        pthread_cond_wait(&q->cond, &q->mutex);
    }
    
    // Add sentence
    strncpy(q->sentences[q->tail], sentence, MAX_SENTENCE_LEN - 1);
    q->sentences[q->tail][MAX_SENTENCE_LEN - 1] = '\0';
    
    q->tail = (q->tail + 1) % MAX_SENTENCES;
    q->count++;
    
    // Signal waiting thread
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

bool queue_pop(sentence_queue_t *q, char *sentence, size_t max_len)
{
    pthread_mutex_lock(&q->mutex);
    
    // Wait if queue is empty and not done 
    while (q->count == 0 && !q->done)
    {
        pthread_cond_wait(&q->cond, &q->mutex);
    }
    
    // If empty and done 
    if (q->count == 0 && q->done)
    {
        pthread_mutex_unlock(&q->mutex);
        return false;
    }
    
    // Get sentence 
    strncpy(sentence, q->sentences[q->head], max_len - 1);
    sentence[max_len - 1] = '\0';
    
    q->head = (q->head + 1) % MAX_SENTENCES;
    q->count--;
    
    // Signal waiting thread 
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
    return true;
}

void queue_mark_done(sentence_queue_t *q)
{
    pthread_mutex_lock(&q->mutex);
    q->done = true;
    pthread_cond_broadcast(&q->cond);  // Wake all waiting threads
    pthread_mutex_unlock(&q->mutex);
}

void queue_cleanup(sentence_queue_t *q)
{
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
}

void queue_reset(sentence_queue_t *q)
{
    pthread_mutex_lock(&q->mutex);
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->done = false;
    pthread_mutex_unlock(&q->mutex);
}


// Audio queue implementation 
void audio_queue_init(audio_queue_t *q)
{
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->done = false;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

void audio_queue_push(audio_queue_t *q, const char *filename)
{
    pthread_mutex_lock(&q->mutex);
    
    // Wait if queue is full 
    while (q->count >= MAX_SENTENCES)
    {
        pthread_cond_wait(&q->cond, &q->mutex);
    }
    
    // Add filename 
    strncpy(q->filenames[q->tail], filename, 255);
    q->filenames[q->tail][255] = '\0';
    
    q->tail = (q->tail + 1) % MAX_SENTENCES;
    q->count++;
    
    // Signal waiting thread 
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

bool audio_queue_pop(audio_queue_t *q, char *filename, size_t max_len)
{
    pthread_mutex_lock(&q->mutex);
    
    // Wait if queue is empty and not done 
    while (q->count == 0 && !q->done)
    {
        pthread_cond_wait(&q->cond, &q->mutex);
    }
    
    // If empty and done, return false 
    if (q->count == 0 && q->done)
    {
        pthread_mutex_unlock(&q->mutex);
        return false;
    }
    
    // Get filename 
    strncpy(filename, q->filenames[q->head], max_len - 1);
    filename[max_len - 1] = '\0';
    
    q->head = (q->head + 1) % MAX_SENTENCES;
    q->count--;
    
    // Signal waiting producers 
    pthread_cond_signal(&q->cond);
    
    pthread_mutex_unlock(&q->mutex);
    return true;
}

void audio_queue_mark_done(audio_queue_t *q)
{
    pthread_mutex_lock(&q->mutex);
    q->done = true;
    pthread_cond_broadcast(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

void audio_queue_reset(audio_queue_t *q)
{
    pthread_mutex_lock(&q->mutex);
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->done = false;
    pthread_mutex_unlock(&q->mutex);
}

void audio_queue_cleanup(audio_queue_t *q)
{
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
}
