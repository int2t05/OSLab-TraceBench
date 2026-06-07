/*
 * 文件作用：解析调度模块的标准输入样例。
 * 设计原因：输入格式校验和算法执行分离，便于测试错误输入时定位问题。
 */
#include "scheduler.h"

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

/*
 * 去掉行首和行尾空白。
 * 这里直接在读取缓冲区上处理，是因为输入行只在 parser 内部使用，不需要保留原始格式。
 */
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

static int parse_process_count(const char *line, int *count)
{
    char extra;
    return sscanf(line, "process_count = %d %c", count, &extra) == 1 && *count > 0;
}

static int parse_time_quantum(const char *line, int *time_quantum)
{
    char extra;
    return sscanf(line, "time_quantum = %d %c", time_quantum, &extra) == 1 &&
           *time_quantum > 0;
}

/*
 * 解析单个进程行。
 * 使用较大的临时 name 缓冲区，是为了能明确拒绝超过 31 字符的名称，而不是静默截断。
 */
static int parse_process_line(const char *line, Process *process, int order)
{
    char name_buffer[128];
    char extra;
    size_t name_length;

    if (sscanf(line, "%127s arrival=%d burst=%d priority=%d %c", name_buffer,
               &process->arrival, &process->burst, &process->priority, &extra) != 4) {
        return -1;
    }
    name_length = strlen(name_buffer);
    if (name_length >= NAME_LEN || process->arrival < 0 || process->burst <= 0) {
        return -1;
    }

    memcpy(process->name, name_buffer, name_length + 1);
    process->remaining = process->burst;
    process->start_time = -1;
    process->finish_time = -1;
    process->input_order = order;
    return 0;
}

/*
 * 解析完整输入。
 * 第一条有效行必须是 process_count，后续允许 time_quantum 和进程行；这种限制来自 PRD 的固定样例格式。
 */
int parse_scheduler_input(FILE *in, Process **processes, int *count, int *time_quantum,
                          char *error, size_t error_size)
{
    char line_buffer[512];
    char *line;
    int parsed_count = 0;
    int process_index = 0;

    *processes = NULL;
    *count = 0;
    *time_quantum = 0;

    while (fgets(line_buffer, sizeof(line_buffer), in) != NULL) {
        line = trim(line_buffer);
        if (*line == '\0') {
            continue;
        }
        if (!parse_process_count(line, count)) {
            set_error(error, error_size, "first line must be process_count = N with N > 0");
            return -1;
        }
        parsed_count = 1;
        break;
    }

    if (!parsed_count) {
        set_error(error, error_size, "missing process_count");
        return -1;
    }

    *processes = calloc((size_t)*count, sizeof(Process));
    if (*processes == NULL) {
        set_error(error, error_size, "failed to allocate processes");
        return -1;
    }

    while (fgets(line_buffer, sizeof(line_buffer), in) != NULL) {
        line = trim(line_buffer);
        if (*line == '\0') {
            continue;
        }
        if (strncmp(line, "time_quantum", strlen("time_quantum")) == 0) {
            if (!parse_time_quantum(line, time_quantum)) {
                set_error(error, error_size, "time_quantum must be greater than 0");
                free(*processes);
                *processes = NULL;
                return -1;
            }
            continue;
        }

        if (process_index >= *count) {
            set_error(error, error_size, "too many process lines");
            free(*processes);
            *processes = NULL;
            return -1;
        }
        if (parse_process_line(line, &(*processes)[process_index], process_index) != 0) {
            set_error(error, error_size,
                      "invalid process line, expected NAME arrival=A burst=B priority=P");
            free(*processes);
            *processes = NULL;
            return -1;
        }
        process_index++;
    }

    if (process_index != *count) {
        set_error(error, error_size, "process line count does not match process_count");
        free(*processes);
        *processes = NULL;
        return -1;
    }

    return 0;
}
