/*
 * 文件作用：实现 scheduler 命令行入口，负责参数解析、输入解析、算法分发和错误输出。
 * 设计原因：main.c 只协调流程，调度算法保留在 scheduler.c，避免入口文件混入算法细节。
 */
#include "scheduler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_help(const char *program)
{
    printf("usage: %s --algorithm fcfs|sjf|rr|priority < tests/sample.txt\n", program);
    printf("input:\n");
    printf("  process_count = N\n");
    printf("  time_quantum = Q    # required by rr\n");
    printf("  NAME arrival=A burst=B priority=P\n");
}

/*
 * 解析算法参数。
 * 这里只接受 PRD 固定的 --algorithm 形式，是为了避免扩展出未文档化的 CLI 形态。
 */
static int parse_algorithm_arg(int argc, char **argv, const char **algorithm)
{
    int i;

    *algorithm = NULL;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            return 1;
        }
        if (strcmp(argv[i], "--algorithm") == 0 && i + 1 < argc) {
            *algorithm = argv[++i];
            continue;
        }
        return -1;
    }

    if (*algorithm == NULL) {
        return -1;
    }
    if (strcmp(*algorithm, "fcfs") != 0 && strcmp(*algorithm, "sjf") != 0 &&
        strcmp(*algorithm, "rr") != 0 && strcmp(*algorithm, "priority") != 0) {
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    const char *algorithm;
    Process *processes = NULL;
    Timeline timeline;
    int count = 0;
    int time_quantum = 0;
    int arg_status;
    int run_status = 0;
    char error[ERROR_LEN];

    arg_status = parse_algorithm_arg(argc, argv, &algorithm);
    if (arg_status == 1) {
        print_help(argv[0]);
        return 0;
    }
    if (arg_status != 0) {
        fprintf(stderr, "error: invalid arguments\n");
        print_help(argv[0]);
        return 1;
    }

    if (parse_scheduler_input(stdin, &processes, &count, &time_quantum, error, sizeof(error)) != 0) {
        fprintf(stderr, "error: %s\n", error);
        return 1;
    }

    if (strcmp(algorithm, "rr") == 0 && time_quantum <= 0) {
        fprintf(stderr, "error: rr requires positive time_quantum\n");
        free(processes);
        return 1;
    }

    if (timeline_init(&timeline) != 0) {
        fprintf(stderr, "error: failed to allocate timeline\n");
        free(processes);
        return 1;
    }

    reset_processes(processes, count);
    if (strcmp(algorithm, "fcfs") == 0) {
        run_status = run_fcfs(processes, count, &timeline);
    } else if (strcmp(algorithm, "sjf") == 0) {
        run_status = run_sjf(processes, count, &timeline);
    } else if (strcmp(algorithm, "rr") == 0) {
        run_status = run_rr(processes, count, time_quantum, &timeline);
    } else {
        run_status = run_priority(processes, count, &timeline);
    }

    if (run_status != 0) {
        fprintf(stderr, "error: failed to run scheduler\n");
        timeline_free(&timeline);
        free(processes);
        return 1;
    }

    print_scheduler_result(algorithm, processes, count, &timeline);
    timeline_free(&timeline);
    free(processes);
    return 0;
}
