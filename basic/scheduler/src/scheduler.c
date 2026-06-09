/*
 * 文件作用：实现 FCFS、SJF、RR 和非抢占式优先级调度算法。
 * 设计原因：四种算法共享进程状态和时间线结构，集中在本文件中便于保持平局规则一致。
 */
#include "scheduler.h"

#include <stdlib.h>
#include <string.h>

int timeline_init(Timeline *timeline)
{
    timeline->count = 0;
    timeline->capacity = 16;
    timeline->items = malloc(sizeof(Segment) * (size_t)timeline->capacity);
    return timeline->items == NULL ? -1 : 0;
}

/*
 * 追加甘特图时间段。
 * 不合并相邻同名段，是因为 RR 需要保留每个时间片边界用于调度行为分析。
 */
int timeline_append(Timeline *timeline, const char *name, int start, int end)
{
    Segment *new_items;
    if (end <= start) {
        return 0;
    }
    if (timeline->count == timeline->capacity) {
        timeline->capacity *= 2;
        new_items = realloc(timeline->items, sizeof(Segment) * (size_t)timeline->capacity);
        if (new_items == NULL) {
            return -1;
        }
        timeline->items = new_items;
    }

    snprintf(timeline->items[timeline->count].name, NAME_LEN, "%s", name);
    timeline->items[timeline->count].start = start;
    timeline->items[timeline->count].end = end;
    timeline->count++;
    return 0;
}

void timeline_free(Timeline *timeline)
{
    free(timeline->items);
    timeline->items = NULL;
    timeline->count = 0;
    timeline->capacity = 0;
}

void reset_processes(Process *processes, int count)
{
    int i;
    for (i = 0; i < count; i++) {
        processes[i].remaining = processes[i].burst;
        processes[i].start_time = -1;
        processes[i].finish_time = -1;
    }
}

static int choose_fcfs(const Process *processes, const int *done, int count)
{
    int best = -1;
    int i;

    for (i = 0; i < count; i++) {
        if (done[i]) {
            continue;
        }
        if (best == -1 || processes[i].arrival < processes[best].arrival ||
            (processes[i].arrival == processes[best].arrival &&
             processes[i].input_order < processes[best].input_order)) {
            best = i;
        }
    }
    return best;
}

/*
 * 运行 FCFS。
 * 选择逻辑只比较到达时间和输入顺序，符合 PRD 对先来先服务的固定平局规则。
 */
int run_fcfs(Process *processes, int count, Timeline *timeline)
{
    int *done = calloc((size_t)count, sizeof(int));
    int now = 0;
    int completed;
    int idx;

    if (done == NULL) {
        return -1;
    }

    for (completed = 0; completed < count; completed++) {
        idx = choose_fcfs(processes, done, count);
        if (idx < 0) {
            free(done);
            return -1;
        }
        if (now < processes[idx].arrival) {
            if (timeline_append(timeline, "IDLE", now, processes[idx].arrival) != 0) {
                free(done);
                return -1;
            }
            now = processes[idx].arrival;
        }
        processes[idx].start_time = now;
        if (timeline_append(timeline, processes[idx].name, now, now + processes[idx].burst) != 0) {
            free(done);
            return -1;
        }
        now += processes[idx].burst;
        processes[idx].remaining = 0;
        processes[idx].finish_time = now;
        done[idx] = 1;
    }

    free(done);
    return 0;
}

static int unfinished_exists(const Process *processes, int count)
{
    int i;
    for (i = 0; i < count; i++) {
        if (processes[i].remaining > 0) {
            return 1;
        }
    }
    return 0;
}

static int next_arrival_time(const Process *processes, int count)
{
    int i;
    int next = -1;

    for (i = 0; i < count; i++) {
        if (processes[i].remaining <= 0) {
            continue;
        }
        if (next == -1 || processes[i].arrival < next) {
            next = processes[i].arrival;
        }
    }
    return next;
}

static int choose_sjf(const Process *processes, int count, int now)
{
    int best = -1;
    int i;

    for (i = 0; i < count; i++) {
        if (processes[i].remaining <= 0 || processes[i].arrival > now) {
            continue;
        }
        if (best == -1 || processes[i].burst < processes[best].burst ||
            (processes[i].burst == processes[best].burst &&
             (processes[i].arrival < processes[best].arrival ||
              (processes[i].arrival == processes[best].arrival &&
               processes[i].input_order < processes[best].input_order)))) {
            best = i;
        }
    }
    return best;
}

/*
 * 运行非抢占式 SJF。
 * 这里只在 CPU 空闲时选择一次进程，是因为文档固定为非抢占式短作业优先。
 */
int run_sjf(Process *processes, int count, Timeline *timeline)
{
    int now = 0;
    int idx;
    int next;

    while (unfinished_exists(processes, count)) {
        idx = choose_sjf(processes, count, now);
        if (idx < 0) {
            next = next_arrival_time(processes, count);
            if (next < 0) {
                return -1;
            }
            if (timeline_append(timeline, "IDLE", now, next) != 0) {
                return -1;
            }
            now = next;
            continue;
        }
        processes[idx].start_time = now;
        if (timeline_append(timeline, processes[idx].name, now, now + processes[idx].burst) != 0) {
            return -1;
        }
        now += processes[idx].burst;
        processes[idx].remaining = 0;
        processes[idx].finish_time = now;
    }
    return 0;
}

static int choose_priority(const Process *processes, int count, int now)
{
    int best = -1;
    int i;

    for (i = 0; i < count; i++) {
        if (processes[i].remaining <= 0 || processes[i].arrival > now) {
            continue;
        }
        if (best == -1 || processes[i].priority < processes[best].priority ||
            (processes[i].priority == processes[best].priority &&
             (processes[i].arrival < processes[best].arrival ||
              (processes[i].arrival == processes[best].arrival &&
               processes[i].input_order < processes[best].input_order)))) {
            best = i;
        }
    }
    return best;
}

/*
 * 运行非抢占式优先级调度。
 * 优先级数值越小表示越高，平局规则沿用到达时间和输入顺序。
 */
int run_priority(Process *processes, int count, Timeline *timeline)
{
    int now = 0;
    int idx;
    int next;

    while (unfinished_exists(processes, count)) {
        idx = choose_priority(processes, count, now);
        if (idx < 0) {
            next = next_arrival_time(processes, count);
            if (next < 0) {
                return -1;
            }
            if (timeline_append(timeline, "IDLE", now, next) != 0) {
                return -1;
            }
            now = next;
            continue;
        }
        processes[idx].start_time = now;
        if (timeline_append(timeline, processes[idx].name, now, now + processes[idx].burst) != 0) {
            return -1;
        }
        now += processes[idx].burst;
        processes[idx].remaining = 0;
        processes[idx].finish_time = now;
    }
    return 0;
}

static int compare_arrival_order(const void *left, const void *right, void *context)
{
    const Process *processes = context;
    int li = *(const int *)left;
    int ri = *(const int *)right;

    if (processes[li].arrival != processes[ri].arrival) {
        return processes[li].arrival - processes[ri].arrival;
    }
    return processes[li].input_order - processes[ri].input_order;
}

static void sort_by_arrival(Process *processes, int *order, int count)
{
    int i;
    int j;

    for (i = 1; i < count; i++) {
        int key = order[i];
        j = i - 1;
        while (j >= 0 && compare_arrival_order(&order[j], &key, processes) > 0) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = key;
    }
}

static int enqueue(int *queue, int *length, int capacity, int value)
{
    if (*length >= capacity) {
        return -1;
    }
    queue[*length] = value;
    (*length)++;
    return 0;
}

static int dequeue(int *queue, int *length)
{
    int value = queue[0];
    int i;

    for (i = 1; i < *length; i++) {
        queue[i - 1] = queue[i];
    }
    (*length)--;
    return value;
}

/*
 * 运行 RR。
 * 时间片结束后先接纳这段时间内新到达的进程，再把未完成进程放回队尾，匹配 PRD 的队列顺序。
 */
int run_rr(Process *processes, int count, int time_quantum, Timeline *timeline)
{
    int *arrival_order = malloc(sizeof(int) * (size_t)count);
    int *queue = malloc(sizeof(int) * (size_t)count);
    int queue_length = 0;
    int next_to_arrive = 0;
    int completed = 0;
    int now = 0;
    int i;

    if (arrival_order == NULL || queue == NULL) {
        free(arrival_order);
        free(queue);
        return -1;
    }

    for (i = 0; i < count; i++) {
        arrival_order[i] = i;
    }
    sort_by_arrival(processes, arrival_order, count);

    while (completed < count) {
        int idx;
        int run_time;
        int start;

        while (next_to_arrive < count &&
               processes[arrival_order[next_to_arrive]].arrival <= now) {
            if (enqueue(queue, &queue_length, count, arrival_order[next_to_arrive]) != 0) {
                free(arrival_order);
                free(queue);
                return -1;
            }
            next_to_arrive++;
        }

        if (queue_length == 0) {
            int next_time;
            if (next_to_arrive >= count) {
                break;
            }
            next_time = processes[arrival_order[next_to_arrive]].arrival;
            if (timeline_append(timeline, "IDLE", now, next_time) != 0) {
                free(arrival_order);
                free(queue);
                return -1;
            }
            now = next_time;
            continue;
        }

        idx = dequeue(queue, &queue_length);
        if (processes[idx].start_time < 0) {
            processes[idx].start_time = now;
        }
        start = now;
        run_time = processes[idx].remaining < time_quantum ? processes[idx].remaining : time_quantum;
        now += run_time;
        processes[idx].remaining -= run_time;

        if (timeline_append(timeline, processes[idx].name, start, now) != 0) {
            free(arrival_order);
            free(queue);
            return -1;
        }

        while (next_to_arrive < count &&
               processes[arrival_order[next_to_arrive]].arrival <= now) {
            if (enqueue(queue, &queue_length, count, arrival_order[next_to_arrive]) != 0) {
                free(arrival_order);
                free(queue);
                return -1;
            }
            next_to_arrive++;
        }

        if (processes[idx].remaining > 0) {
            if (enqueue(queue, &queue_length, count, idx) != 0) {
                free(arrival_order);
                free(queue);
                return -1;
            }
        } else {
            processes[idx].finish_time = now;
            completed++;
        }
    }

    free(arrival_order);
    free(queue);
    return completed == count ? 0 : -1;
}
