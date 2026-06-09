/*
 * 文件作用：解析同步模块的命令行参数并填充默认值。
 * 设计原因：默认参数来自 PRD，集中解析可以避免不同问题对 count 等字段解释不一致。
 */
#include "sync.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_sync_help(const char *program)
{
    printf("usage: %s --problem producer_consumer --producers 2 --consumers 2 --buffer-size 4 --count 10\n",
           program);
    printf("       %s --problem readers_writers --readers 3 --writers 2 --count 5\n", program);
    printf("       %s --problem dining_philosophers --count 3\n", program);
}

static int parse_positive(const char *text, int *value)
{
    char *endptr;
    long parsed = strtol(text, &endptr, 10);
    if (*text == '\0' || *endptr != '\0' || parsed <= 0 || parsed > 1000000L) {
        return -1;
    }
    *value = (int)parsed;
    return 0;
}

static void set_defaults(SyncConfig *config)
{
    config->problem = PROBLEM_PRODUCER_CONSUMER;
    config->producers = 2;
    config->consumers = 2;
    config->buffer_size = 4;
    config->readers = 3;
    config->writers = 2;
    config->count = 10;
}

/*
 * 解析 CLI 参数。
 * 未出现 --problem 视为错误，因为用户必须明确选择要运行的同步问题。
 */
int parse_sync_args(int argc, char **argv, SyncConfig *config)
{
    int i;
    int have_problem = 0;

    set_defaults(config);
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            return 1;
        }
        if (strcmp(argv[i], "--problem") == 0 && i + 1 < argc) {
            const char *problem = argv[++i];
            have_problem = 1;
            if (strcmp(problem, "producer_consumer") == 0) {
                config->problem = PROBLEM_PRODUCER_CONSUMER;
                continue;
            }
            if (strcmp(problem, "readers_writers") == 0) {
                config->problem = PROBLEM_READERS_WRITERS;
                continue;
            }
            if (strcmp(problem, "dining_philosophers") == 0) {
                config->problem = PROBLEM_DINING_PHILOSOPHERS;
                continue;
            }
            return -1;
        }
        if (strcmp(argv[i], "--producers") == 0 && i + 1 < argc) {
            if (parse_positive(argv[++i], &config->producers) != 0) {
                return -1;
            }
            continue;
        }
        if (strcmp(argv[i], "--consumers") == 0 && i + 1 < argc) {
            if (parse_positive(argv[++i], &config->consumers) != 0) {
                return -1;
            }
            continue;
        }
        if (strcmp(argv[i], "--buffer-size") == 0 && i + 1 < argc) {
            if (parse_positive(argv[++i], &config->buffer_size) != 0) {
                return -1;
            }
            continue;
        }
        if (strcmp(argv[i], "--readers") == 0 && i + 1 < argc) {
            if (parse_positive(argv[++i], &config->readers) != 0) {
                return -1;
            }
            continue;
        }
        if (strcmp(argv[i], "--writers") == 0 && i + 1 < argc) {
            if (parse_positive(argv[++i], &config->writers) != 0) {
                return -1;
            }
            continue;
        }
        if (strcmp(argv[i], "--count") == 0 && i + 1 < argc) {
            if (parse_positive(argv[++i], &config->count) != 0) {
                return -1;
            }
            continue;
        }
        return -1;
    }

    return have_problem ? 0 : -1;
}
