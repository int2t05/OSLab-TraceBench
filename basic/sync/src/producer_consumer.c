/*
 * 文件作用：实现生产者-消费者问题。
 * 设计原因：使用 mutex 和条件变量可以直接展示缓冲区满/空时的阻塞和唤醒机制。
 */
#include "sync.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    PcBuffer *buffer;
    int id;
} PcThreadArg;

static void *producer_thread(void *arg)
{
    PcThreadArg *thread_arg = arg;
    PcBuffer *buffer = thread_arg->buffer;
    int i;

    for (i = 0; i < buffer->target_per_producer; i++) {
        pthread_mutex_lock(&buffer->mutex);
        while (buffer->size == buffer->capacity) {
            log_event("producer_consumer", thread_arg->id, "wait_not_full", buffer->size);
            pthread_cond_wait(&buffer->not_full, &buffer->mutex);
        }
        buffer->items[buffer->tail] = thread_arg->id * 1000 + i;
        buffer->tail = (buffer->tail + 1) % buffer->capacity;
        buffer->size++;
        buffer->produced_total++;
        log_event("producer_consumer", thread_arg->id, "produce", buffer->size);
        pthread_cond_signal(&buffer->not_empty);
        pthread_mutex_unlock(&buffer->mutex);
    }
    return NULL;
}

static void *consumer_thread(void *arg)
{
    PcThreadArg *thread_arg = arg;
    PcBuffer *buffer = thread_arg->buffer;

    while (1) {
        pthread_mutex_lock(&buffer->mutex);
        while (buffer->size == 0 && buffer->consumed_total < buffer->target_total) {
            log_event("producer_consumer", thread_arg->id, "wait_not_empty", buffer->size);
            pthread_cond_wait(&buffer->not_empty, &buffer->mutex);
        }
        if (buffer->consumed_total >= buffer->target_total) {
            pthread_cond_broadcast(&buffer->not_empty);
            pthread_mutex_unlock(&buffer->mutex);
            break;
        }
        buffer->head = (buffer->head + 1) % buffer->capacity;
        buffer->size--;
        buffer->consumed_total++;
        log_event("producer_consumer", thread_arg->id, "consume", buffer->size);
        pthread_cond_signal(&buffer->not_full);
        if (buffer->consumed_total >= buffer->target_total) {
            pthread_cond_broadcast(&buffer->not_empty);
        }
        pthread_mutex_unlock(&buffer->mutex);
    }
    return NULL;
}

/*
 * 运行生产者-消费者。
 * 消费者以全局 consumed_total 为退出条件，避免生产者和消费者数量不相等时残留线程。
 */
int run_producer_consumer(const SyncConfig *config)
{
    PcBuffer buffer;
    pthread_t *producer_threads;
    pthread_t *consumer_threads;
    PcThreadArg *producer_args;
    PcThreadArg *consumer_args;
    int i;

    buffer.items = calloc((size_t)config->buffer_size, sizeof(int));
    producer_threads = calloc((size_t)config->producers, sizeof(pthread_t));
    consumer_threads = calloc((size_t)config->consumers, sizeof(pthread_t));
    producer_args = calloc((size_t)config->producers, sizeof(PcThreadArg));
    consumer_args = calloc((size_t)config->consumers, sizeof(PcThreadArg));
    if (buffer.items == NULL || producer_threads == NULL || consumer_threads == NULL ||
        producer_args == NULL || consumer_args == NULL) {
        fprintf(stderr, "error: failed to allocate producer-consumer state\n");
        free(buffer.items);
        free(producer_threads);
        free(consumer_threads);
        free(producer_args);
        free(consumer_args);
        return -1;
    }

    buffer.capacity = config->buffer_size;
    buffer.head = 0;
    buffer.tail = 0;
    buffer.size = 0;
    buffer.producers = config->producers;
    buffer.produced_total = 0;
    buffer.consumed_total = 0;
    buffer.target_total = config->producers * config->count;
    buffer.target_per_producer = config->count;
    pthread_mutex_init(&buffer.mutex, NULL);
    pthread_cond_init(&buffer.not_full, NULL);
    pthread_cond_init(&buffer.not_empty, NULL);

    for (i = 0; i < config->consumers; i++) {
        consumer_args[i].buffer = &buffer;
        consumer_args[i].id = i;
        pthread_create(&consumer_threads[i], NULL, consumer_thread, &consumer_args[i]);
    }
    for (i = 0; i < config->producers; i++) {
        producer_args[i].buffer = &buffer;
        producer_args[i].id = i;
        pthread_create(&producer_threads[i], NULL, producer_thread, &producer_args[i]);
    }
    for (i = 0; i < config->producers; i++) {
        pthread_join(producer_threads[i], NULL);
    }
    pthread_mutex_lock(&buffer.mutex);
    pthread_cond_broadcast(&buffer.not_empty);
    pthread_mutex_unlock(&buffer.mutex);
    for (i = 0; i < config->consumers; i++) {
        pthread_join(consumer_threads[i], NULL);
    }

    printf("produced_total: %d\n", buffer.produced_total);
    printf("consumed_total: %d\n", buffer.consumed_total);
    printf("buffer_final_size: %d\n", buffer.size);

    pthread_cond_destroy(&buffer.not_empty);
    pthread_cond_destroy(&buffer.not_full);
    pthread_mutex_destroy(&buffer.mutex);
    free(buffer.items);
    free(producer_threads);
    free(consumer_threads);
    free(producer_args);
    free(consumer_args);
    return 0;
}
