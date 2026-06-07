/*
 * 文件作用：实现动态分区 FF/BF 分配、回收和相邻空闲分区合并。
 * 设计原因：链表能直接表达分区分裂和合并，避免为课程模拟引入复杂索引结构。
 */
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Partition *create_partition(int start, int size, int free_flag, const char *owner)
{
    Partition *partition = malloc(sizeof(*partition));
    if (partition == NULL) {
        return NULL;
    }
    partition->start = start;
    partition->size = size;
    partition->free = free_flag;
    if (owner != NULL) {
        snprintf(partition->owner, sizeof(partition->owner), "%s", owner);
    } else {
        partition->owner[0] = '\0';
    }
    partition->next = NULL;
    return partition;
}

static void free_partitions(Partition *head)
{
    while (head != NULL) {
        Partition *next = head->next;
        free(head);
        head = next;
    }
}

static Partition *find_owner(Partition *head, const char *owner)
{
    Partition *current;
    for (current = head; current != NULL; current = current->next) {
        if (!current->free && strcmp(current->owner, owner) == 0) {
            return current;
        }
    }
    return NULL;
}

static Partition *select_free_partition(Partition *head, int size, PartitionAlgorithm algorithm)
{
    Partition *current;
    Partition *best = NULL;

    for (current = head; current != NULL; current = current->next) {
        if (!current->free || current->size < size) {
            continue;
        }
        if (algorithm == PARTITION_FF) {
            return current;
        }
        if (best == NULL || current->size - size < best->size - size ||
            (current->size - size == best->size - size && current->start < best->start)) {
            best = current;
        }
    }
    return best;
}

/*
 * 分配分区。
 * 若空闲分区大于申请大小，则原节点变为已分配节点，剩余空间作为后继空闲节点保留。
 */
static int allocate_partition(Partition *head, const char *owner, int size,
                              PartitionAlgorithm algorithm)
{
    Partition *target;
    Partition *remaining;

    if (find_owner(head, owner) != NULL) {
        printf("error: duplicate allocation for %s\n", owner);
        return -1;
    }

    target = select_free_partition(head, size, algorithm);
    if (target == NULL) {
        printf("error: not enough memory for %s\n", owner);
        return -1;
    }

    if (target->size > size) {
        remaining = create_partition(target->start + size, target->size - size, 1, NULL);
        if (remaining == NULL) {
            printf("error: failed to split partition\n");
            return -1;
        }
        remaining->next = target->next;
        target->next = remaining;
    }

    target->size = size;
    target->free = 0;
    snprintf(target->owner, sizeof(target->owner), "%s", owner);
    return 0;
}

/*
 * 合并相邻空闲分区。
 * 每次回收后从链表头扫描，保证释放前后两个方向的相邻空闲区都能被合并。
 */
static void merge_free_partitions(Partition *head)
{
    Partition *current = head;

    while (current != NULL && current->next != NULL) {
        if (current->free && current->next->free) {
            Partition *next = current->next;
            current->size += next->size;
            current->next = next->next;
            free(next);
            continue;
        }
        current = current->next;
    }
}

static int release_partition(Partition *head, const char *owner)
{
    Partition *target = find_owner(head, owner);
    if (target == NULL) {
        printf("error: unknown allocation %s\n", owner);
        return -1;
    }

    target->free = 1;
    target->owner[0] = '\0';
    merge_free_partitions(head);
    return 0;
}

/*
 * 执行动态分区命令流。
 * 单条操作失败后继续输出当前状态，便于报告中观察错误发生时的内存表。
 */
int run_partition(const PartitionInput *input, PartitionAlgorithm algorithm)
{
    Partition *head = create_partition(0, input->memory_size, 1, NULL);
    int had_error = 0;
    int i;

    if (head == NULL) {
        fprintf(stderr, "error: failed to initialize memory\n");
        return -1;
    }

    for (i = 0; i < input->op_count; i++) {
        const PartitionOp *op = &input->ops[i];
        int status;

        printf("operation %d: ", i + 1);
        if (op->type == OP_ALLOC) {
            printf("alloc %s %d\n", op->name, op->size);
            status = allocate_partition(head, op->name, op->size, algorithm);
        } else {
            printf("free %s\n", op->name);
            status = release_partition(head, op->name);
        }
        printf("status: %s\n", status == 0 ? "success" : "failed");
        print_partition_state(head);
        if (status != 0) {
            had_error = 1;
        }
    }

    free_partitions(head);
    return had_error ? -1 : 0;
}
