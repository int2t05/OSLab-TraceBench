/*
 * 文件作用：实现 memory 命令行入口，负责 mode/algorithm 参数解析和子系统分发。
 * 设计原因：入口只处理 CLI 和错误返回，FF/BF/FIFO/LRU 细节分别放入专门文件。
 */
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_help(const char *program)
{
    printf("usage: %s --mode partition --algorithm ff|bf < tests/partition.txt\n", program);
    printf("       %s --mode paging --algorithm fifo|lru < tests/pages.txt\n", program);
}

/*
 * 解析固定 CLI 参数。
 * 只接受文档声明的参数组合，避免增加未文档化的入口形态。
 */
static int parse_args(int argc, char **argv, const char **mode, const char **algorithm)
{
    int i;

    *mode = NULL;
    *algorithm = NULL;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            return 1;
        }
        if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            *mode = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "--algorithm") == 0 && i + 1 < argc) {
            *algorithm = argv[++i];
            continue;
        }
        return -1;
    }

    if (*mode == NULL || *algorithm == NULL) {
        return -1;
    }
    return 0;
}

static int run_partition_mode(const char *algorithm, char *error)
{
    PartitionInput input;
    PartitionAlgorithm partition_algorithm;
    int status;

    if (strcmp(algorithm, "ff") == 0) {
        partition_algorithm = PARTITION_FF;
    } else if (strcmp(algorithm, "bf") == 0) {
        partition_algorithm = PARTITION_BF;
    } else {
        fprintf(stderr, "error: partition mode requires algorithm ff or bf\n");
        return 1;
    }

    if (parse_partition_input(stdin, &input, error, ERROR_LEN) != 0) {
        fprintf(stderr, "error: %s\n", error);
        return 1;
    }

    printf("mode: partition\n");
    printf("algorithm: %s\n", algorithm);
    status = run_partition(&input, partition_algorithm);
    free_partition_input(&input);
    return status == 0 ? 0 : 1;
}

static int run_paging_mode(const char *algorithm, char *error)
{
    PagingInput input;
    PagingResult result;
    PagingAlgorithm paging_algorithm;
    int status;

    if (strcmp(algorithm, "fifo") == 0) {
        paging_algorithm = PAGING_FIFO;
    } else if (strcmp(algorithm, "lru") == 0) {
        paging_algorithm = PAGING_LRU;
    } else {
        fprintf(stderr, "error: paging mode requires algorithm fifo or lru\n");
        return 1;
    }

    if (parse_paging_input(stdin, &input, error, ERROR_LEN) != 0) {
        fprintf(stderr, "error: %s\n", error);
        return 1;
    }

    status = run_paging(&input, paging_algorithm, &result);
    if (status == 0) {
        print_paging_result(algorithm, &input, &result);
        free_paging_result(&result);
    } else {
        fprintf(stderr, "error: failed to run paging algorithm\n");
    }
    free_paging_input(&input);
    return status == 0 ? 0 : 1;
}

int main(int argc, char **argv)
{
    const char *mode;
    const char *algorithm;
    char error[ERROR_LEN];
    int arg_status;

    arg_status = parse_args(argc, argv, &mode, &algorithm);
    if (arg_status == 1) {
        print_help(argv[0]);
        return 0;
    }
    if (arg_status != 0) {
        fprintf(stderr, "error: invalid arguments\n");
        print_help(argv[0]);
        return 1;
    }

    if (strcmp(mode, "partition") == 0) {
        return run_partition_mode(algorithm, error);
    }
    if (strcmp(mode, "paging") == 0) {
        return run_paging_mode(algorithm, error);
    }

    fprintf(stderr, "error: mode must be partition or paging\n");
    return 1;
}
