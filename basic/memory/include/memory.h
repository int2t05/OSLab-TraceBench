/*
 * 文件作用：声明内存管理模型模块的共享结构和函数。
 * 设计原因：动态分区和页面置换属于同一 CLI，但状态模型不同，共用头文件用于约束边界。
 */
#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdio.h>

#define NAME_LEN 32
#define ERROR_LEN 256

typedef enum {
    PARTITION_FF,
    PARTITION_BF
} PartitionAlgorithm;

typedef enum {
    PAGING_FIFO,
    PAGING_LRU
} PagingAlgorithm;

typedef struct Partition {
    int start;
    int size;
    int free;
    char owner[NAME_LEN];
    struct Partition *next;
} Partition;

typedef enum {
    OP_ALLOC,
    OP_FREE
} PartitionOpType;

typedef struct {
    PartitionOpType type;
    char name[NAME_LEN];
    int size;
} PartitionOp;

typedef struct {
    int memory_size;
    PartitionOp *ops;
    int op_count;
    int op_capacity;
} PartitionInput;

typedef struct {
    int page;
    int *frames;
    int frame_count;
    int fault;
    int evicted;
} PagingStep;

typedef struct {
    int frame_count;
    int *references;
    int reference_count;
} PagingInput;

typedef struct {
    PagingStep *steps;
    int step_count;
    int faults;
} PagingResult;

int parse_partition_input(FILE *in, PartitionInput *input, char *error, size_t error_size);
int parse_paging_input(FILE *in, PagingInput *input, char *error, size_t error_size);
void free_partition_input(PartitionInput *input);
void free_paging_input(PagingInput *input);

int run_partition(const PartitionInput *input, PartitionAlgorithm algorithm);
int run_paging(const PagingInput *input, PagingAlgorithm algorithm, PagingResult *result);
void free_paging_result(PagingResult *result);

void print_partition_state(const Partition *head);
void print_paging_result(const char *algorithm, const PagingInput *input, const PagingResult *result);

#endif
