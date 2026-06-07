/*
 * 文件作用：实现 oslabctl 用户态工具，封装 /proc/oslab_monitor 的读取和 PID 写入。
 * 设计原因：用户态工具只做 proc 访问，不重新格式化输出，确保与直接 cat /proc 结果一致。
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define PROC_DIR "/proc/oslab_monitor"
#define PROC_OVERVIEW "/proc/oslab_monitor/overview"
#define PROC_TASKS "/proc/oslab_monitor/tasks"
#define PROC_PID "/proc/oslab_monitor/pid"

static void print_help(const char *program)
{
    printf("usage: %s --help\n", program);
    printf("       %s overview\n", program);
    printf("       %s tasks\n", program);
    printf("       sudo %s pid <PID>\n", program);
}

static int ensure_module_loaded(void)
{
    struct stat st;
    if (stat(PROC_DIR, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "error: %s not found, load oslab_monitor module first\n", PROC_DIR);
        return -1;
    }
    return 0;
}

/*
 * 原样打印 proc 文件内容。
 * 使用 fread 循环而不是固定一次性缓冲，避免 tasks 进程列表较长时被截断。
 */
static int print_file(const char *path)
{
    FILE *fp;
    char buffer[4096];
    size_t nread;

    if (ensure_module_loaded() != 0) {
        return -1;
    }

    fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "error: failed to open %s: %s\n", path, strerror(errno));
        return -1;
    }

    while ((nread = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        if (fwrite(buffer, 1, nread, stdout) != nread) {
            fprintf(stderr, "error: failed to write stdout\n");
            fclose(fp);
            return -1;
        }
    }
    if (ferror(fp)) {
        fprintf(stderr, "error: failed to read %s: %s\n", path, strerror(errno));
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}

static int write_text_file(const char *path, const char *text)
{
    FILE *fp;

    if (ensure_module_loaded() != 0) {
        return -1;
    }

    fp = fopen(path, "w");
    if (fp == NULL) {
        fprintf(stderr, "error: failed to open %s: %s\n", path, strerror(errno));
        if (errno == EACCES || errno == EPERM) {
            fprintf(stderr, "error: try sudo ./oslabctl pid <PID>\n");
        }
        return -1;
    }
    if (fprintf(fp, "%s\n", text) < 0) {
        fprintf(stderr, "error: failed to write %s: %s\n", path, strerror(errno));
        fclose(fp);
        return -1;
    }
    if (fclose(fp) != 0) {
        fprintf(stderr, "error: failed to close %s: %s\n", path, strerror(errno));
        return -1;
    }
    return 0;
}

static int parse_pid_arg(const char *text)
{
    char *endptr;
    long value = strtol(text, &endptr, 10);
    if (*text == '\0' || *endptr != '\0' || value <= 0 || value > 2147483647L) {
        return -1;
    }
    return 0;
}

static int cmd_pid(const char *pid_text)
{
    if (parse_pid_arg(pid_text) != 0) {
        fprintf(stderr, "error: pid must be a positive integer\n");
        return -1;
    }
    if (write_text_file(PROC_PID, pid_text) != 0) {
        return -1;
    }
    return print_file(PROC_PID);
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        print_help(argv[0]);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "overview") == 0) {
        return print_file(PROC_OVERVIEW) == 0 ? 0 : 1;
    }
    if (argc == 2 && strcmp(argv[1], "tasks") == 0) {
        return print_file(PROC_TASKS) == 0 ? 0 : 1;
    }
    if (argc == 3 && strcmp(argv[1], "pid") == 0) {
        return cmd_pid(argv[2]) == 0 ? 0 : 1;
    }

    fprintf(stderr, "error: invalid arguments\n");
    print_help(argv[0]);
    return 1;
}
