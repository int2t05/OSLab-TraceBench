/*
 * 文件作用：实现 sync 命令行入口，解析参数并分发到具体同步问题。
 * 设计原因：入口层不直接操作线程状态，保证三个同步问题可以独立验证和维护。
 */
#include "sync.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    SyncConfig config;
    int status;

    status = parse_sync_args(argc, argv, &config);
    if (status == 1) {
        print_sync_help(argv[0]);
        return 0;
    }
    if (status != 0) {
        fprintf(stderr, "error: invalid arguments\n");
        print_sync_help(argv[0]);
        return 1;
    }

    switch (config.problem) {
    case PROBLEM_PRODUCER_CONSUMER:
        return run_producer_consumer(&config) == 0 ? 0 : 1;
    case PROBLEM_READERS_WRITERS:
        return run_readers_writers(&config) == 0 ? 0 : 1;
    case PROBLEM_DINING_PHILOSOPHERS:
        return run_dining_philosophers(&config) == 0 ? 0 : 1;
    }

    fprintf(stderr, "error: unknown problem\n");
    return 1;
}
