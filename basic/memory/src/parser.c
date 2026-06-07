/*
 * 文件作用：解析内存管理模块的动态分区和页面置换输入。
 * 设计原因：两个 mode 的输入格式不同，集中解析可以统一错误信息和资源释放。
 */
#include "memory.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error_size > 0) {
        snprintf(error, error_size, "%s", message);
    }
}

static char *trim(char *line)
{
    char *end;

    while (isspace((unsigned char)*line)) {
        line++;
    }
    if (*line == '\0') {
        return line;
    }
    end = line + strlen(line) - 1;
    while (end > line && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    return line;
}

static int append_partition_op(PartitionInput *input, const PartitionOp *op)
{
    PartitionOp *new_ops;

    if (input->op_count == input->op_capacity) {
        int new_capacity = input->op_capacity == 0 ? 8 : input->op_capacity * 2;
        new_ops = realloc(input->ops, sizeof(PartitionOp) * (size_t)new_capacity);
        if (new_ops == NULL) {
            return -1;
        }
        input->ops = new_ops;
        input->op_capacity = new_capacity;
    }
    input->ops[input->op_count++] = *op;
    return 0;
}

/*
 * 解析动态分区输入。
 * 第一条有效行固定为 memory_size，是为了让后续 alloc/free 的合法性有明确上下文。
 */
int parse_partition_input(FILE *in, PartitionInput *input, char *error, size_t error_size)
{
    char line_buffer[512];
    char *line;
    int have_memory_size = 0;

    memset(input, 0, sizeof(*input));

    while (fgets(line_buffer, sizeof(line_buffer), in) != NULL) {
        line = trim(line_buffer);
        if (*line == '\0') {
            continue;
        }
        if (!have_memory_size) {
            char extra;
            if (sscanf(line, "memory_size = %d %c", &input->memory_size, &extra) != 1 ||
                input->memory_size <= 0) {
                set_error(error, error_size, "first line must be memory_size = SIZE with SIZE > 0");
                return -1;
            }
            have_memory_size = 1;
            continue;
        }

        if (strncmp(line, "alloc ", 6) == 0) {
            PartitionOp op;
            char name_buffer[128];
            char extra;
            if (sscanf(line, "alloc %127s %d %c", name_buffer, &op.size, &extra) != 2 ||
                strlen(name_buffer) >= NAME_LEN || op.size <= 0) {
                set_error(error, error_size, "invalid alloc command");
                free_partition_input(input);
                return -1;
            }
            op.type = OP_ALLOC;
            memcpy(op.name, name_buffer, strlen(name_buffer) + 1);
            if (append_partition_op(input, &op) != 0) {
                set_error(error, error_size, "failed to allocate partition operations");
                free_partition_input(input);
                return -1;
            }
            continue;
        }

        if (strncmp(line, "free ", 5) == 0) {
            PartitionOp op;
            char name_buffer[128];
            char extra;
            if (sscanf(line, "free %127s %c", name_buffer, &extra) != 1 ||
                strlen(name_buffer) >= NAME_LEN) {
                set_error(error, error_size, "invalid free command");
                free_partition_input(input);
                return -1;
            }
            op.type = OP_FREE;
            op.size = 0;
            memcpy(op.name, name_buffer, strlen(name_buffer) + 1);
            if (append_partition_op(input, &op) != 0) {
                set_error(error, error_size, "failed to allocate partition operations");
                free_partition_input(input);
                return -1;
            }
            continue;
        }

        set_error(error, error_size, "unknown partition command");
        free_partition_input(input);
        return -1;
    }

    if (!have_memory_size) {
        set_error(error, error_size, "missing memory_size");
        return -1;
    }
    return 0;
}

static int append_reference(PagingInput *input, int value)
{
    int *new_refs;
    int new_count = input->reference_count + 1;

    new_refs = realloc(input->references, sizeof(int) * (size_t)new_count);
    if (new_refs == NULL) {
        return -1;
    }
    input->references = new_refs;
    input->references[input->reference_count++] = value;
    return 0;
}

/*
 * 解析页面置换输入。
 * reference_string 至少需要一个非负页号，否则缺页率没有定义。
 */
int parse_paging_input(FILE *in, PagingInput *input, char *error, size_t error_size)
{
    char line_buffer[1024];
    char *line;
    char *cursor;
    char *endptr;
    int have_frame_count = 0;
    int have_reference_string = 0;

    memset(input, 0, sizeof(*input));

    while (fgets(line_buffer, sizeof(line_buffer), in) != NULL) {
        line = trim(line_buffer);
        if (*line == '\0') {
            continue;
        }
        if (!have_frame_count) {
            char extra;
            if (sscanf(line, "frame_count = %d %c", &input->frame_count, &extra) != 1 ||
                input->frame_count <= 0) {
                set_error(error, error_size, "first line must be frame_count = N with N > 0");
                return -1;
            }
            have_frame_count = 1;
            continue;
        }

        if (strncmp(line, "reference_string =", 18) != 0) {
            set_error(error, error_size, "missing reference_string");
            free_paging_input(input);
            return -1;
        }
        have_reference_string = 1;
        cursor = trim(line + 18);
        while (*cursor != '\0') {
            long value;
            while (isspace((unsigned char)*cursor)) {
                cursor++;
            }
            if (*cursor == '\0') {
                break;
            }
            value = strtol(cursor, &endptr, 10);
            if (cursor == endptr || value < 0 || value > 2147483647L) {
                set_error(error, error_size, "reference_string must contain non-negative pages");
                free_paging_input(input);
                return -1;
            }
            if (append_reference(input, (int)value) != 0) {
                set_error(error, error_size, "failed to allocate reference string");
                free_paging_input(input);
                return -1;
            }
            cursor = endptr;
        }
        break;
    }

    if (!have_frame_count) {
        set_error(error, error_size, "missing frame_count");
        return -1;
    }
    if (!have_reference_string || input->reference_count == 0) {
        set_error(error, error_size, "reference_string must contain at least one page");
        free_paging_input(input);
        return -1;
    }
    return 0;
}

void free_partition_input(PartitionInput *input)
{
    free(input->ops);
    input->ops = NULL;
    input->op_count = 0;
    input->op_capacity = 0;
    input->memory_size = 0;
}

void free_paging_input(PagingInput *input)
{
    free(input->references);
    input->references = NULL;
    input->reference_count = 0;
    input->frame_count = 0;
}
