/*
 * 文件作用：集中输出调度结果、进程统计和平均指标。
 * 设计原因：稳定输出字段便于 Bash 测试、课程截图和文档验收复用。
 */
#include "scheduler.h"

#include <stdio.h>

static int turnaround_time(const Process *process)
{
    return process->finish_time - process->arrival;
}

static int waiting_time(const Process *process)
{
    return turnaround_time(process) - process->burst;
}

/*
 * 打印调度结果。
 * 平均值统一保留两位小数，避免不同模块或报告截图出现格式漂移。
 */
void print_scheduler_result(const char *algorithm, const Process *processes, int count,
                            const Timeline *timeline)
{
    double total_waiting = 0.0;
    double total_turnaround = 0.0;
    double total_weighted = 0.0;
    int i;

    printf("algorithm: %s\n", algorithm);
    printf("timeline:");
    for (i = 0; i < timeline->count; i++) {
        printf(" %s[%d,%d]", timeline->items[i].name, timeline->items[i].start,
               timeline->items[i].end);
    }
    printf("\n");

    printf("name arrival burst priority start finish waiting turnaround weighted_turnaround\n");
    for (i = 0; i < count; i++) {
        int turnaround = turnaround_time(&processes[i]);
        int waiting = waiting_time(&processes[i]);
        double weighted = (double)turnaround / processes[i].burst;

        total_waiting += waiting;
        total_turnaround += turnaround;
        total_weighted += weighted;

        printf("%s %d %d %d %d %d %d %d %.2f\n", processes[i].name, processes[i].arrival,
               processes[i].burst, processes[i].priority, processes[i].start_time,
               processes[i].finish_time, waiting, turnaround, weighted);
    }

    printf("average_waiting: %.2f\n", total_waiting / count);
    printf("average_turnaround: %.2f\n", total_turnaround / count);
    printf("average_weighted_turnaround: %.2f\n", total_weighted / count);
}
