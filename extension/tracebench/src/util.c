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
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

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
    char tmp[TB_PATH_LEN];
    int len;

    if (path == NULL || path[0] == '\0') {
        tb_print_error("output path must not be empty");
        return -1;
    }
    if ((int)strlen(path) >= TB_PATH_LEN) {
        tb_print_error("output path is too long");
        return -1;
    }

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = (int)strlen(tmp);
    while (len > 1 && tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
        len--;
    }

    for (int i = 1; tmp[i] != '\0'; i++) {
        if (tmp[i] == '/') {
            tmp[i] = '\0';
            if (mkdir(tmp, 0775) != 0 && errno != EEXIST) {
                tb_print_error("failed to create directory %s: %s", tmp, strerror(errno));
                return -1;
            }
            tmp[i] = '/';
        }
    }

    if (mkdir(tmp, 0775) != 0 && errno != EEXIST) {
        tb_print_error("failed to create directory %s: %s", tmp, strerror(errno));
        return -1;
    }

    return 0;
}

int tb_join_path(char *dest, int dest_size, const char *dir, const char *name)
{
    int written;

    if (dest == NULL || dir == NULL || name == NULL || dest_size <= 0) {
        tb_print_error("invalid path join arguments");
        return -1;
    }

    if (strstr(name, "/") != NULL || strcmp(name, "..") == 0) {
        tb_print_error("invalid file name for path join: %s", name);
        return -1;
    }

    written = snprintf(dest, (size_t)dest_size, "%s%s%s",
                       dir, (dir[0] != '\0' && dir[strlen(dir) - 1] == '/') ? "" : "/", name);
    if (written < 0 || written >= dest_size) {
        tb_print_error("path is too long: %s/%s", dir, name);
        return -1;
    }

    return 0;
}

int tb_parent_dir(char *dest, int dest_size, const char *path)
{
    char *slash;

    if (dest == NULL || path == NULL || dest_size <= 0 || (int)strlen(path) >= dest_size) {
        tb_print_error("invalid parent path arguments");
        return -1;
    }

    snprintf(dest, (size_t)dest_size, "%s", path);
    slash = strrchr(dest, '/');
    if (slash == NULL) {
        snprintf(dest, (size_t)dest_size, ".");
        return 0;
    }
    if (slash == dest) {
        slash[1] = '\0';
        return 0;
    }
    *slash = '\0';
    return 0;
}

int tb_read_text_file(const char *path, char *buffer, int buffer_size)
{
    FILE *file;
    size_t nread;

    if (buffer == NULL || buffer_size <= 0) {
        tb_print_error("invalid read buffer");
        return -1;
    }

    file = fopen(path, "r");
    if (file == NULL) {
        tb_print_error("failed to open %s: %s", path, strerror(errno));
        return -1;
    }

    nread = fread(buffer, 1, (size_t)buffer_size - 1, file);
    if (ferror(file)) {
        tb_print_error("failed to read %s: %s", path, strerror(errno));
        fclose(file);
        return -1;
    }
    buffer[nread] = '\0';
    fclose(file);
    return 0;
}

int tb_write_text_file(const char *path, const char *text)
{
    FILE *file;

    file = fopen(path, "w");
    if (file == NULL) {
        tb_print_error("failed to open %s for writing: %s", path, strerror(errno));
        return -1;
    }

    if (fputs(text, file) == EOF) {
        tb_print_error("failed to write %s: %s", path, strerror(errno));
        fclose(file);
        return -1;
    }

    if (fclose(file) != 0) {
        tb_print_error("failed to close %s: %s", path, strerror(errno));
        return -1;
    }

    return 0;
}

int tb_path_readable(const char *path)
{
    return access(path, R_OK) == 0;
}

int tb_remove_empty_dir(const char *path)
{
    if (rmdir(path) != 0) {
        return -1;
    }
    return 0;
}

long long tb_now_millis(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }

    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

int tb_is_root(void)
{
    return geteuid() == 0;
}
