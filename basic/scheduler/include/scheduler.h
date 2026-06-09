/*
 * 文件作用：声明调度模型模块内部共享的数据结构和函数接口。
 * 设计原因：调度模块按 parser、算法和输出分文件实现，共用头文件可以保持模块边界清晰。
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stddef.h>
#include <stdio.h>

#define NAME_LEN 32
#define ERROR_LEN 256

typedef struct {
    char name[NAME_LEN];
    int arrival;
    int burst;
    int priority;
    int remaining;
    int start_time;
    int finish_time;
    int input_order;
} Process;

typedef struct {
    char name[NAME_LEN];
    int start;
    int end;
} Segment;

typedef struct {
    Segment *items;
    int count;
    int capacity;
} Timeline;

int parse_scheduler_input(FILE *in, Process **processes, int *count, int *time_quantum,
                          char *error, size_t error_size);
int timeline_init(Timeline *timeline);
int timeline_append(Timeline *timeline, const char *name, int start, int end);
void timeline_free(Timeline *timeline);
void reset_processes(Process *processes, int count);

int run_fcfs(Process *processes, int count, Timeline *timeline);
int run_sjf(Process *processes, int count, Timeline *timeline);
int run_priority(Process *processes, int count, Timeline *timeline);
int run_rr(Process *processes, int count, int time_quantum, Timeline *timeline);

void print_scheduler_result(const char *algorithm, const Process *processes, int count,
                            const Timeline *timeline);

#endif
