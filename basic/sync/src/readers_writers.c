/*
 * 文件作用：实现写者优先的读者-写者问题。
 * 设计原因：写者优先可以避免写者长期饥饿，同时条件变量能清楚表达进入条件。
 */
#include "sync.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    RwState *state;
    int id;
    int count;
} RwThreadArg;

static void *reader_thread(void *arg)
{
    RwThreadArg *thread_arg = arg;
    RwState *state = thread_arg->state;
    int i;

    for (i = 0; i < thread_arg->count; i++) {
        pthread_mutex_lock(&state->mutex);
        while (state->active_writers > 0 || state->waiting_writers > 0) {
            log_event("readers_writers", thread_arg->id, "reader_wait", state->shared_value);
            pthread_cond_wait(&state->can_read, &state->mutex);
        }
        state->active_readers++;
        log_event("readers_writers", thread_arg->id, "read_start", state->shared_value);
        pthread_mutex_unlock(&state->mutex);

        pthread_mutex_lock(&state->mutex);
        state->read_total++;
        state->active_readers--;
        log_event("readers_writers", thread_arg->id, "read_end", state->read_total);
        if (state->active_readers == 0) {
            pthread_cond_signal(&state->can_write);
        }
        pthread_mutex_unlock(&state->mutex);
    }
    return NULL;
}

static void *writer_thread(void *arg)
{
    RwThreadArg *thread_arg = arg;
    RwState *state = thread_arg->state;
    int i;

    for (i = 0; i < thread_arg->count; i++) {
        pthread_mutex_lock(&state->mutex);
        state->waiting_writers++;
        while (state->active_writers > 0 || state->active_readers > 0) {
            log_event("readers_writers", thread_arg->id, "writer_wait", state->shared_value);
            pthread_cond_wait(&state->can_write, &state->mutex);
        }
        state->waiting_writers--;
        state->active_writers = 1;
        log_event("readers_writers", thread_arg->id, "write_start", state->shared_value);
        state->shared_value++;
        state->write_total++;
        state->active_writers = 0;
        log_event("readers_writers", thread_arg->id, "write_end", state->shared_value);
        if (state->waiting_writers > 0) {
            pthread_cond_signal(&state->can_write);
        } else {
            pthread_cond_broadcast(&state->can_read);
        }
        pthread_mutex_unlock(&state->mutex);
    }
    return NULL;
}

int run_readers_writers(const SyncConfig *config)
{
    RwState state;
    pthread_t *reader_threads;
    pthread_t *writer_threads;
    RwThreadArg *reader_args;
    RwThreadArg *writer_args;
    int i;

    reader_threads = calloc((size_t)config->readers, sizeof(pthread_t));
    writer_threads = calloc((size_t)config->writers, sizeof(pthread_t));
    reader_args = calloc((size_t)config->readers, sizeof(RwThreadArg));
    writer_args = calloc((size_t)config->writers, sizeof(RwThreadArg));
    if (reader_threads == NULL || writer_threads == NULL || reader_args == NULL ||
        writer_args == NULL) {
        fprintf(stderr, "error: failed to allocate readers-writers state\n");
        free(reader_threads);
        free(writer_threads);
        free(reader_args);
        free(writer_args);
        return -1;
    }

    state.active_readers = 0;
    state.active_writers = 0;
    state.waiting_writers = 0;
    state.shared_value = 0;
    state.read_total = 0;
    state.write_total = 0;
    pthread_mutex_init(&state.mutex, NULL);
    pthread_cond_init(&state.can_read, NULL);
    pthread_cond_init(&state.can_write, NULL);

    for (i = 0; i < config->readers; i++) {
        reader_args[i].state = &state;
        reader_args[i].id = i;
        reader_args[i].count = config->count;
        pthread_create(&reader_threads[i], NULL, reader_thread, &reader_args[i]);
    }
    for (i = 0; i < config->writers; i++) {
        writer_args[i].state = &state;
        writer_args[i].id = i;
        writer_args[i].count = config->count;
        pthread_create(&writer_threads[i], NULL, writer_thread, &writer_args[i]);
    }
    for (i = 0; i < config->readers; i++) {
        pthread_join(reader_threads[i], NULL);
    }
    for (i = 0; i < config->writers; i++) {
        pthread_join(writer_threads[i], NULL);
    }

    printf("read_total: %d\n", state.read_total);
    printf("write_total: %d\n", state.write_total);
    printf("final_shared_value: %d\n", state.shared_value);

    pthread_cond_destroy(&state.can_write);
    pthread_cond_destroy(&state.can_read);
    pthread_mutex_destroy(&state.mutex);
    free(reader_threads);
    free(writer_threads);
    free(reader_args);
    free(writer_args);
    return 0;
}
