/*
 * 文件作用：实现哲学家进餐问题。
 * 设计原因：限制最多 4 个哲学家同时竞争筷子，可以破坏循环等待条件并避免死锁。
 */
#include "sync.h"

#include <stdio.h>

typedef struct {
    DiningState *state;
    int id;
} DiningThreadArg;

static void enter_room(DiningState *state, int id)
{
    pthread_mutex_lock(&state->room_mutex);
    while (state->in_room >= PHILOSOPHER_COUNT - 1) {
        log_event("dining_philosophers", id, "wait_room", state->in_room);
        pthread_cond_wait(&state->room_available, &state->room_mutex);
    }
    state->in_room++;
    log_event("dining_philosophers", id, "enter_room", state->in_room);
    pthread_mutex_unlock(&state->room_mutex);
}

static void leave_room(DiningState *state, int id)
{
    pthread_mutex_lock(&state->room_mutex);
    state->in_room--;
    log_event("dining_philosophers", id, "leave_room", state->in_room);
    pthread_cond_signal(&state->room_available);
    pthread_mutex_unlock(&state->room_mutex);
}

static void *philosopher_thread(void *arg)
{
    DiningThreadArg *thread_arg = arg;
    DiningState *state = thread_arg->state;
    int id = thread_arg->id;
    int left = id;
    int right = (id + 1) % PHILOSOPHER_COUNT;
    int i;

    for (i = 0; i < state->target_count; i++) {
        enter_room(state, id);
        pthread_mutex_lock(&state->chopsticks[left]);
        log_event("dining_philosophers", id, "lock_left", left);
        pthread_mutex_lock(&state->chopsticks[right]);
        log_event("dining_philosophers", id, "lock_right", right);
        state->eat_count[id]++;
        log_event("dining_philosophers", id, "eat", state->eat_count[id]);
        pthread_mutex_unlock(&state->chopsticks[right]);
        pthread_mutex_unlock(&state->chopsticks[left]);
        log_event("dining_philosophers", id, "release_chopsticks", i + 1);
        leave_room(state, id);
    }
    return NULL;
}

int run_dining_philosophers(const SyncConfig *config)
{
    DiningState state;
    pthread_t threads[PHILOSOPHER_COUNT];
    DiningThreadArg args[PHILOSOPHER_COUNT];
    int i;

    state.in_room = 0;
    state.target_count = config->count;
    pthread_mutex_init(&state.room_mutex, NULL);
    pthread_cond_init(&state.room_available, NULL);
    for (i = 0; i < PHILOSOPHER_COUNT; i++) {
        state.eat_count[i] = 0;
        pthread_mutex_init(&state.chopsticks[i], NULL);
    }

    for (i = 0; i < PHILOSOPHER_COUNT; i++) {
        args[i].state = &state;
        args[i].id = i;
        pthread_create(&threads[i], NULL, philosopher_thread, &args[i]);
    }
    for (i = 0; i < PHILOSOPHER_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    for (i = 0; i < PHILOSOPHER_COUNT; i++) {
        printf("philosopher_%d_eat_count: %d\n", i, state.eat_count[i]);
    }
    printf("deadlock_detected: no\n");

    for (i = 0; i < PHILOSOPHER_COUNT; i++) {
        pthread_mutex_destroy(&state.chopsticks[i]);
    }
    pthread_cond_destroy(&state.room_available);
    pthread_mutex_destroy(&state.room_mutex);
    return 0;
}
