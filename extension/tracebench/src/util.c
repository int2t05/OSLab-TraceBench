/*
 * 文件作用：提供 TraceBench 内部通用的错误输出、参数校验和帮助文本。
 * 设计原因：这些能力会被 CLI、采样、报告和 cleanup 复用，放在 util.c 能避免各模块重复实现。
 */

#include "tracebench.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void tb_print_error(const char *fmt, ...)
{
    va_list args;

    fprintf(stderr, "error: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

int tb_parse_positive_int(const char *text, const char *name, int *value)
{
    char *end = NULL;
    long parsed;

    if (text == NULL || text[0] == '\0') {
        tb_print_error("%s must be a positive integer", name);
        return -1;
    }

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed <= 0 || parsed > 2147483647L) {
        tb_print_error("%s must be a positive integer", name);
        return -1;
    }

    *value = (int)parsed;
    return 0;
}

int tb_is_valid_cgroup_name(const char *name)
{
    if (name == NULL || name[0] == '\0') {
        return 0;
    }

    for (int i = 0; name[i] != '\0'; i++) {
        unsigned char c = (unsigned char)name[i];
        if (!isalnum(c) && c != '_' && c != '-') {
            return 0;
        }
    }

    return 1;
}

const char *tb_profile_name(TbProfile profile)
{
    switch (profile) {
    case TB_PROFILE_CPU:
        return "cpu";
    case TB_PROFILE_MEMORY:
        return "memory";
    case TB_PROFILE_IO:
        return "io";
    }
    return "unknown";
}

void tb_print_help(const char *argv0)
{
    printf("usage:\n");
    printf("  %s --help\n", argv0);
    printf("  %s run --profile cpu|memory|io --duration N --sample-interval N --output PATH [options]\n", argv0);
    printf("  %s report --input PATH --output PATH\n", argv0);
    printf("  %s cleanup [--cgroup-name NAME]\n", argv0);
    printf("\n");
    printf("run options:\n");
    printf("  --cpu-workers N          CPU workload worker count, default 2\n");
    printf("  --memory-mb N            memory workload size, default 128\n");
    printf("  --io-mb N                I/O workload size, default 64\n");
    printf("  --cgroup-name NAME       cgroup v2 namespace, default oslab_tracebench\n");
    printf("  --no-cgroup              low-permission demo mode\n");
    printf("  --with-oslab-monitor     require /proc/oslab_monitor/overview\n");
}

int tb_mkdir_p(const char *path)
{
    (void)path;
    tb_print_error("run output directory support is not implemented yet");
    return -1;
}

int tb_read_text_file(const char *path, char *buffer, int buffer_size)
{
    (void)path;
    (void)buffer;
    (void)buffer_size;
    tb_print_error("file reading is not implemented yet");
    return -1;
}

int tb_write_text_file(const char *path, const char *text)
{
    (void)path;
    (void)text;
    tb_print_error("file writing is not implemented yet");
    return -1;
}

long long tb_now_millis(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }

    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}
