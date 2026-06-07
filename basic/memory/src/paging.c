/*
 * 文件作用：实现 FIFO 和 LRU 页面置换。
 * 设计原因：页面置换状态适合用数组表达，能直接输出每一步页框状态和缺页统计。
 */
#include "memory.h"

#include <stdlib.h>

static int find_page(const int *frames, int frame_count, int page)
{
    int i;
    for (i = 0; i < frame_count; i++) {
        if (frames[i] == page) {
            return i;
        }
    }
    return -1;
}

static int find_empty_frame(const int *frames, int frame_count)
{
    int i;
    for (i = 0; i < frame_count; i++) {
        if (frames[i] == -1) {
            return i;
        }
    }
    return -1;
}

static int select_victim(const int *values, int frame_count)
{
    int victim = 0;
    int i;

    for (i = 1; i < frame_count; i++) {
        if (values[i] < values[victim]) {
            victim = i;
        }
    }
    return victim;
}

static int copy_frames(PagingStep *step, const int *frames)
{
    int i;
    step->frames = malloc(sizeof(int) * (size_t)step->frame_count);
    if (step->frames == NULL) {
        return -1;
    }
    for (i = 0; i < step->frame_count; i++) {
        step->frames[i] = frames[i];
    }
    return 0;
}

/*
 * 执行页面置换。
 * FIFO 只更新装入时间，LRU 在命中和装入时都更新最近访问时间。
 */
int run_paging(const PagingInput *input, PagingAlgorithm algorithm, PagingResult *result)
{
    int *frames = malloc(sizeof(int) * (size_t)input->frame_count);
    int *loaded_at = calloc((size_t)input->frame_count, sizeof(int));
    int *last_used_at = calloc((size_t)input->frame_count, sizeof(int));
    int i;

    result->steps = calloc((size_t)input->reference_count, sizeof(PagingStep));
    result->step_count = input->reference_count;
    result->faults = 0;

    if (frames == NULL || loaded_at == NULL || last_used_at == NULL || result->steps == NULL) {
        free(frames);
        free(loaded_at);
        free(last_used_at);
        free(result->steps);
        result->steps = NULL;
        return -1;
    }

    for (i = 0; i < input->frame_count; i++) {
        frames[i] = -1;
        loaded_at[i] = -1;
        last_used_at[i] = -1;
    }

    for (i = 0; i < input->reference_count; i++) {
        int page = input->references[i];
        int frame_index = find_page(frames, input->frame_count, page);
        PagingStep *step = &result->steps[i];

        step->page = page;
        step->frame_count = input->frame_count;
        step->fault = 0;
        step->evicted = -1;

        if (frame_index >= 0) {
            if (algorithm == PAGING_LRU) {
                last_used_at[frame_index] = i;
            }
        } else {
            int target = find_empty_frame(frames, input->frame_count);
            step->fault = 1;
            result->faults++;
            if (target < 0) {
                target = algorithm == PAGING_FIFO ? select_victim(loaded_at, input->frame_count)
                                                  : select_victim(last_used_at, input->frame_count);
                step->evicted = frames[target];
            }
            frames[target] = page;
            loaded_at[target] = i;
            last_used_at[target] = i;
        }

        if (copy_frames(step, frames) != 0) {
            free(frames);
            free(loaded_at);
            free(last_used_at);
            free_paging_result(result);
            return -1;
        }
    }

    free(frames);
    free(loaded_at);
    free(last_used_at);
    return 0;
}

void free_paging_result(PagingResult *result)
{
    int i;

    if (result->steps != NULL) {
        for (i = 0; i < result->step_count; i++) {
            free(result->steps[i].frames);
        }
    }
    free(result->steps);
    result->steps = NULL;
    result->step_count = 0;
    result->faults = 0;
}
