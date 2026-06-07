/*
 * 文件作用：声明同步实验模块的参数、共享状态和运行函数。
 * 设计原因：三个经典同步问题共享 CLI 和输出约定，但同步状态彼此独立。
 */
#ifndef SYNC_H
#define SYNC_H

#include <pthread.h>

#define PHILOSOPHER_COUNT 5

typedef enum {
    PROBLEM_PRODUCER_CONSUMER,
    PROBLEM_READERS_WRITERS,
    PROBLEM_DINING_PHILOSOPHERS
} SyncProblem;

typedef struct {
    SyncProblem problem;
    int producers;
    int consumers;
    int buffer_size;
    int readers;
    int writers;
    int count;
} SyncConfig;

typedef struct {
    int *items;
    int capacity;
    int head;
    int tail;
    int size;
    int producers;
    int produced_total;
    int consumed_total;
    int target_total;
    int target_per_producer;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} PcBuffer;

typedef struct {
    int active_readers;
    int active_writers;
    int waiting_writers;
    int shared_value;
    int read_total;
    int write_total;
    pthread_mutex_t mutex;
    pthread_cond_t can_read;
    pthread_cond_t can_write;
} RwState;

typedef struct {
    pthread_mutex_t chopsticks[PHILOSOPHER_COUNT];
    pthread_mutex_t room_mutex;
    pthread_cond_t room_available;
    int in_room;
    int eat_count[PHILOSOPHER_COUNT];
    int target_count;
} DiningState;

int parse_sync_args(int argc, char **argv, SyncConfig *config);
void print_sync_help(const char *program);
void log_event(const char *problem, int id, const char *action, int value);

int run_producer_consumer(const SyncConfig *config);
int run_readers_writers(const SyncConfig *config);
int run_dining_philosophers(const SyncConfig *config);

#endif
