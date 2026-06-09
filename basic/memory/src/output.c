/*
 * 文件作用：集中输出内存模块的分区表和页面置换过程。
 * 设计原因：输出字段稳定后，测试脚本和分析记录可以直接复用同一份结果。
 */
#include "memory.h"

#include <stdio.h>

void print_partition_state(const Partition *head)
{
    const Partition *current;

    printf("allocated_partitions:\n");
    for (current = head; current != NULL; current = current->next) {
        if (!current->free) {
            printf("%s %d %d\n", current->owner, current->start, current->size);
        }
    }

    printf("free_partitions:\n");
    for (current = head; current != NULL; current = current->next) {
        if (current->free) {
            printf("%d %d\n", current->start, current->size);
        }
    }
}

/*
 * 输出页面置换结果。
 * evicted 在命中和装入空页框时固定为 '-'，与 PRD 的字段约定保持一致。
 */
void print_paging_result(const char *algorithm, const PagingInput *input, const PagingResult *result)
{
    int i;
    int j;
    double fault_rate = (double)result->faults / input->reference_count * 100.0;

    printf("mode: paging\n");
    printf("algorithm: %s\n", algorithm);
    for (i = 0; i < result->step_count; i++) {
        const PagingStep *step = &result->steps[i];
        printf("step %d page %d frames:", i + 1, step->page);
        for (j = 0; j < step->frame_count; j++) {
            if (step->frames[j] < 0) {
                printf(" -");
            } else {
                printf(" %d", step->frames[j]);
            }
        }
        printf(" fault: %s evicted: ", step->fault ? "yes" : "no");
        if (step->evicted < 0) {
            printf("-");
        } else {
            printf("%d", step->evicted);
        }
        printf("\n");
    }
    printf("page_faults: %d\n", result->faults);
    printf("fault_rate: %.2f%%\n", fault_rate);
}
