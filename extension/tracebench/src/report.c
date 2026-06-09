/*
 * 文件作用：实现 TraceBench run/report/cleanup 当前阶段的命令边界。
 * 设计原因：运行、汇总和清理共享同一组采样产物，集中在本文件可以保持输出字段一致。
 */

#include "tracebench.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#define TB_REPORT_LINE_LEN 8192
#define TB_SUMMARY_METRIC_COUNT 12

typedef struct {
    const char *name;
    int column;
    int seen;
    unsigned long long first;
    unsigned long long last;
    unsigned long long max;
} TbMetricSummary;

typedef struct {
    int column_count;
    int sample_rows;
    int profile_column;
    int duration_column;
    int interval_column;
    int cgroup_enabled_column;
    int oslab_available_column;
    int oslab_available;
    char profile[TB_VALUE_LEN];
    char duration_sec[TB_VALUE_LEN];
    char sample_interval_sec[TB_VALUE_LEN];
    char cgroup_enabled[TB_VALUE_LEN];
    TbMetricSummary metrics[TB_SUMMARY_METRIC_COUNT];
} TbCsvSummary;

static volatile sig_atomic_t run_interrupted = 0;
static pid_t active_child_pid = -1;

static const char *summary_metric_names[TB_SUMMARY_METRIC_COUNT] = {
    "cpu_some_total",
    "memory_some_total",
    "io_some_total",
    "cgroup_cpu_usage_usec",
    "cgroup_memory_current",
    "cgroup_memory_events_high",
    "cgroup_memory_events_max",
    "cgroup_memory_events_oom",
    "oslab_total_tasks",
    "oslab_running_tasks",
    "oslab_mem_free_kb",
    "oslab_mem_available_kb"
};

static void handle_run_signal(int signo)
{
    (void)signo;
    run_interrupted = 1;
    if (active_child_pid > 0) {
        kill(active_child_pid, SIGTERM);
    }
}

static void read_gcc_version(char *buffer, int size)
{
    FILE *pipe_file;

    snprintf(buffer, (size_t)size, "unavailable");
    pipe_file = popen("gcc --version 2>/dev/null", "r");
    if (pipe_file == NULL) {
        return;
    }

    if (fgets(buffer, size, pipe_file) == NULL) {
        snprintf(buffer, (size_t)size, "unavailable");
    } else {
        buffer[strcspn(buffer, "\r\n")] = '\0';
    }

    pclose(pipe_file);
}

static int append_text(char *buffer, int size, int *used, const char *fmt, ...)
{
    va_list args;
    int written;

    va_start(args, fmt);
    written = vsnprintf(buffer + *used, (size_t)(size - *used), fmt, args);
    va_end(args);
    if (written < 0 || written >= size - *used) {
        tb_print_error("output text is too large");
        return -1;
    }

    *used += written;
    return 0;
}

static void init_csv_summary(TbCsvSummary *summary)
{
    memset(summary, 0, sizeof(*summary));
    summary->profile_column = -1;
    summary->duration_column = -1;
    summary->interval_column = -1;
    summary->cgroup_enabled_column = -1;
    summary->oslab_available_column = -1;
    snprintf(summary->profile, sizeof(summary->profile), "unknown");
    snprintf(summary->duration_sec, sizeof(summary->duration_sec), "unknown");
    snprintf(summary->sample_interval_sec, sizeof(summary->sample_interval_sec), "unknown");
    snprintf(summary->cgroup_enabled, sizeof(summary->cgroup_enabled), "unknown");

    for (int i = 0; i < TB_SUMMARY_METRIC_COUNT; i++) {
        summary->metrics[i].name = summary_metric_names[i];
        summary->metrics[i].column = -1;
    }
}

static int split_csv_line(char *line, char **fields, int max_fields)
{
    int count = 0;
    char *field = line;

    line[strcspn(line, "\r\n")] = '\0';
    while (field != NULL && count < max_fields) {
        char *comma = strchr(field, ',');

        if (comma != NULL) {
            *comma = '\0';
        }
        fields[count++] = field;
        field = comma == NULL ? NULL : comma + 1;
    }

    return count;
}

static void copy_summary_value(char *dest, int size, const char *value)
{
    if (value != NULL && value[0] != '\0') {
        snprintf(dest, (size_t)size, "%s", value);
    }
}

static void map_summary_columns(TbCsvSummary *summary, char **fields, int count)
{
    summary->column_count = count;
    for (int i = 0; i < count; i++) {
        if (strcmp(fields[i], "profile") == 0) {
            summary->profile_column = i;
        } else if (strcmp(fields[i], "duration_sec") == 0) {
            summary->duration_column = i;
        } else if (strcmp(fields[i], "sample_interval_sec") == 0) {
            summary->interval_column = i;
        } else if (strcmp(fields[i], "cgroup_enabled") == 0) {
            summary->cgroup_enabled_column = i;
        } else if (strcmp(fields[i], "oslab_monitor_available") == 0) {
            summary->oslab_available_column = i;
        }

        for (int j = 0; j < TB_SUMMARY_METRIC_COUNT; j++) {
            if (strcmp(fields[i], summary->metrics[j].name) == 0) {
                summary->metrics[j].column = i;
            }
        }
    }
}

static int parse_unsigned_field(const char *text, unsigned long long *value)
{
    char *end = NULL;

    if (text == NULL || text[0] == '\0' || strcmp(text, "NA") == 0) {
        return 0;
    }

    errno = 0;
    *value = strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0') {
        return 0;
    }

    return 1;
}

static void update_metric_summary(TbMetricSummary *metric, const char *field)
{
    unsigned long long value;

    if (!parse_unsigned_field(field, &value)) {
        return;
    }

    if (!metric->seen) {
        metric->first = value;
        metric->max = value;
        metric->seen = 1;
    }
    metric->last = value;
    if (value > metric->max) {
        metric->max = value;
    }
}

static int read_csv_summary(const char *csv_path, TbCsvSummary *summary)
{
    FILE *file;
    char line[TB_REPORT_LINE_LEN];
    char *fields[128];
    int is_header = 1;

    init_csv_summary(summary);
    file = fopen(csv_path, "r");
    if (file == NULL) {
        tb_print_error("failed to open %s: %s", csv_path, strerror(errno));
        return -1;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        int count = split_csv_line(line, fields, 128);

        if (is_header) {
            map_summary_columns(summary, fields, count);
            is_header = 0;
            continue;
        }
        if (count == 0 || fields[0][0] == '\0') {
            continue;
        }
        if (summary->column_count > 0 && count != summary->column_count) {
            tb_print_error("CSV row field count does not match header");
            fclose(file);
            return -1;
        }

        summary->sample_rows++;
        if (summary->sample_rows == 1) {
            if (summary->profile_column >= 0) {
                copy_summary_value(summary->profile, sizeof(summary->profile),
                                   fields[summary->profile_column]);
            }
            if (summary->duration_column >= 0) {
                copy_summary_value(summary->duration_sec, sizeof(summary->duration_sec),
                                   fields[summary->duration_column]);
            }
            if (summary->interval_column >= 0) {
                copy_summary_value(summary->sample_interval_sec, sizeof(summary->sample_interval_sec),
                                   fields[summary->interval_column]);
            }
            if (summary->cgroup_enabled_column >= 0) {
                copy_summary_value(summary->cgroup_enabled, sizeof(summary->cgroup_enabled),
                                   fields[summary->cgroup_enabled_column]);
            }
        }
        if (summary->oslab_available_column >= 0 &&
            strcmp(fields[summary->oslab_available_column], "true") == 0) {
            summary->oslab_available = 1;
        }
        for (int i = 0; i < TB_SUMMARY_METRIC_COUNT; i++) {
            int column = summary->metrics[i].column;

            if (column >= 0 && column < count) {
                update_metric_summary(&summary->metrics[i], fields[column]);
            }
        }
    }

    if (ferror(file)) {
        tb_print_error("failed to read %s: %s", csv_path, strerror(errno));
        fclose(file);
        return -1;
    }
    fclose(file);

    if (is_header || summary->sample_rows == 0) {
        tb_print_error("CSV file has no samples: %s", csv_path);
        return -1;
    }

    return 0;
}

static int append_metric_summary(char *text, int size, int *used, const TbMetricSummary *metric)
{
    long long delta;

    if (!metric->seen) {
        return append_text(text, size, used, "%s: 未采集\n", metric->name);
    }

    if (metric->last >= metric->first) {
        delta = (long long)(metric->last - metric->first);
    } else {
        delta = -(long long)(metric->first - metric->last);
    }

    return append_text(text, size, used,
                       "%s: first=%llu last=%llu delta=%lld max=%llu\n",
                       metric->name,
                       metric->first,
                       metric->last,
                       delta,
                       metric->max);
}

static int open_csv_file(const TbConfig *config, FILE **csv_out)
{
    char csv_path[TB_PATH_LEN];

    if (tb_join_path(csv_path, sizeof(csv_path), config->output_path, "samples.csv") != 0) {
        return -1;
    }

    *csv_out = fopen(csv_path, "w");
    if (*csv_out == NULL) {
        tb_print_error("failed to open %s for writing: %s", csv_path, strerror(errno));
        return -1;
    }

    return 0;
}

static int write_sample_row(FILE *csv, const TbConfig *config, const TbCgroup *cgroup,
                            int sample_index, long long start_ms)
{
    TbSample sample;

    memset(&sample, 0, sizeof(sample));
    sample.sample_index = sample_index;
    sample.elapsed_ms = tb_now_millis() - start_ms;
    sample.profile = config->profile;

    if (tb_read_psi_snapshot(&sample.psi) != 0) {
        return -1;
    }
    if (tb_cgroup_read_stats(cgroup, &sample.cgroup) != 0) {
        return -1;
    }
    if (tb_read_oslab_snapshot(&sample.oslab) != 0) {
        return -1;
    }
    if (tb_write_csv_sample(csv, config, cgroup, &sample) != 0) {
        return -1;
    }
    fflush(csv);
    return 0;
}

static int wait_for_child(pid_t child_pid)
{
    int status;

    while (waitpid(child_pid, &status, 0) < 0) {
        if (errno != EINTR) {
            tb_print_error("failed to wait for workload child: %s", strerror(errno));
            return -1;
        }
    }

    if (run_interrupted) {
        return -1;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        tb_print_error("workload child exited abnormally");
        return -1;
    }

    return 0;
}

/*
 * 采样间隔用 nanosleep 而不是 usleep。
 * c11 + POSIX_C_SOURCE=200809L 下 usleep 可能不暴露声明，nanosleep 可以避免编译告警；
 * 中断信号到来时立即返回，让清理路径尽快回收 workload。
 */
static void sleep_millis(long long sleep_ms)
{
    struct timespec request;

    request.tv_sec = sleep_ms / 1000LL;
    request.tv_nsec = (sleep_ms % 1000LL) * 1000000L;

    while (nanosleep(&request, &request) != 0 && errno == EINTR) {
        if (run_interrupted) {
            break;
        }
    }
}

static int run_sampling_loop(FILE *csv, const TbConfig *config, const TbCgroup *cgroup)
{
    long long start_ms = tb_now_millis();
    long long end_ms = start_ms + (long long)config->duration_sec * 1000LL;
    long long interval_ms = (long long)config->sample_interval_sec * 1000LL;
    int sample_index = 0;

    if (tb_write_csv_header(csv) != 0) {
        return -1;
    }

    while (!run_interrupted) {
        long long now = tb_now_millis();
        long long sleep_ms;

        if (now > end_ms) {
            break;
        }
        if (write_sample_row(csv, config, cgroup, sample_index, start_ms) != 0) {
            return -1;
        }
        sample_index++;

        now = tb_now_millis();
        sleep_ms = interval_ms;
        if (now + sleep_ms > end_ms) {
            sleep_ms = end_ms - now;
        }
        if (sleep_ms <= 0) {
            break;
        }
        sleep_millis(sleep_ms);
    }

    return run_interrupted ? -1 : 0;
}

static int start_workload_child(const TbConfig *config, const TbCgroup *cgroup, pid_t *child_out)
{
    int pipe_fds[2];
    pid_t child_pid;
    char signal_byte = '1';

    if (pipe(pipe_fds) != 0) {
        tb_print_error("failed to create workload pipe: %s", strerror(errno));
        return -1;
    }

    child_pid = fork();
    if (child_pid < 0) {
        tb_print_error("failed to fork workload child: %s", strerror(errno));
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return -1;
    }

    if (child_pid == 0) {
        close(pipe_fds[1]);
        _exit(tb_run_workload_child(config, pipe_fds[0]) == 0 ? 0 : 1);
    }

    close(pipe_fds[0]);
    if (tb_cgroup_add_pid(cgroup, (int)child_pid) != 0) {
        kill(child_pid, SIGTERM);
        close(pipe_fds[1]);
        waitpid(child_pid, NULL, 0);
        return -1;
    }
    if (write(pipe_fds[1], &signal_byte, 1) != 1) {
        tb_print_error("failed to signal workload child: %s", strerror(errno));
        kill(child_pid, SIGTERM);
        close(pipe_fds[1]);
        waitpid(child_pid, NULL, 0);
        return -1;
    }
    close(pipe_fds[1]);

    *child_out = child_pid;
    return 0;
}

int tb_run_command(const TbConfig *config, int argc, char **argv)
{
    TbCgroup cgroup;
    FILE *csv = NULL;
    pid_t child_pid = -1;
    struct sigaction action;
    struct sigaction old_int;
    struct sigaction old_term;
    int result = 1;

    if (config->with_oslab_monitor && !tb_path_readable("/proc/oslab_monitor/overview")) {
        tb_print_error("--with-oslab-monitor requires readable /proc/oslab_monitor/overview");
        return 1;
    }
    if (tb_mkdir_p(config->output_path) != 0) {
        return 1;
    }
    if (tb_write_command_file(config, argc, argv) != 0) {
        return 1;
    }
    if (tb_write_environment_file(config) != 0) {
        return 1;
    }
    if (tb_cgroup_init(config, &cgroup) != 0) {
        return 1;
    }
    if (tb_cgroup_create(&cgroup) != 0) {
        return 1;
    }

    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_run_signal;
    sigemptyset(&action.sa_mask);
    run_interrupted = 0;
    active_child_pid = -1;
    sigaction(SIGINT, &action, &old_int);
    sigaction(SIGTERM, &action, &old_term);

    if (open_csv_file(config, &csv) != 0) {
        goto cleanup;
    }
    if (start_workload_child(config, &cgroup, &child_pid) != 0) {
        goto cleanup;
    }
    active_child_pid = child_pid;
    if (run_sampling_loop(csv, config, &cgroup) != 0) {
        kill(child_pid, SIGTERM);
        goto cleanup;
    }
    if (wait_for_child(child_pid) != 0) {
        goto cleanup;
    }
    child_pid = -1;
    active_child_pid = -1;
    result = 0;

cleanup:
    if (child_pid > 0) {
        kill(child_pid, SIGTERM);
        waitpid(child_pid, NULL, 0);
        active_child_pid = -1;
    }
    if (csv != NULL) {
        fclose(csv);
    }
    if (tb_cgroup_remove_run(&cgroup) != 0 && result == 0) {
        result = 1;
    }
    if (result == 0) {
        char csv_path[TB_PATH_LEN];

        if (tb_join_path(csv_path, sizeof(csv_path), config->output_path, "samples.csv") != 0 ||
            tb_write_summary_file(config, csv_path) != 0) {
            result = 1;
        }
    }
    sigaction(SIGINT, &old_int, NULL);
    sigaction(SIGTERM, &old_term, NULL);
    return result;
}

int tb_report_command(const TbConfig *config)
{
    if (tb_generate_markdown_report(config) != 0) {
        return 1;
    }

    return 0;
}

int tb_cleanup_command(const TbConfig *config)
{
    return tb_cgroup_cleanup_all(config->cgroup_name);
}

int tb_write_command_file(const TbConfig *config, int argc, char **argv)
{
    char path[TB_PATH_LEN];
    char text[TB_LINE_LEN * 2];
    int used = 0;

    if (tb_join_path(path, sizeof(path), config->output_path, "command.txt") != 0) {
        return -1;
    }

    if (append_text(text, sizeof(text), &used, "command:") != 0) {
        return -1;
    }
    for (int i = 0; i < argc; i++) {
        if (append_text(text, sizeof(text), &used, " %s", argv[i]) != 0) {
            return -1;
        }
    }
    if (append_text(text, sizeof(text), &used,
                    "\nprofile: %s\n"
                    "duration_sec: %d\n"
                    "sample_interval_sec: %d\n"
                    "cpu_workers: %d\n"
                    "memory_mb: %d\n"
                    "io_mb: %d\n"
                    "cgroup_name: %s\n"
                    "no_cgroup: %s\n"
                    "with_oslab_monitor: %s\n",
                    tb_profile_name(config->profile),
                    config->duration_sec,
                    config->sample_interval_sec,
                    config->cpu_workers,
                    config->memory_mb,
                    config->io_mb,
                    config->cgroup_name,
                    config->no_cgroup ? "true" : "false",
                    config->with_oslab_monitor ? "true" : "false") != 0) {
        return -1;
    }

    return tb_write_text_file(path, text);
}

int tb_write_environment_file(const TbConfig *config)
{
    char path[TB_PATH_LEN];
    char os_release[TB_LINE_LEN];
    char gcc_version[TB_VALUE_LEN];
    char text[TB_LINE_LEN * 4];
    struct utsname uts;
    int used = 0;

    if (tb_join_path(path, sizeof(path), config->output_path, "environment.txt") != 0) {
        return -1;
    }

    os_release[0] = '\0';
    if (tb_read_text_file("/etc/os-release", os_release, sizeof(os_release)) != 0) {
        snprintf(os_release, sizeof(os_release), "unavailable");
    }
    read_gcc_version(gcc_version, sizeof(gcc_version));

    if (uname(&uts) != 0) {
        snprintf(uts.sysname, sizeof(uts.sysname), "unknown");
        snprintf(uts.release, sizeof(uts.release), "unknown");
        snprintf(uts.version, sizeof(uts.version), "unknown");
        snprintf(uts.machine, sizeof(uts.machine), "unknown");
    }

    if (append_text(text, sizeof(text), &used,
                    "uname: %s %s %s %s\n"
                    "os_release:\n%s\n"
                    "gcc: %s\n"
                    "cgroup_v2: %s\n"
                    "psi_cpu: %s\n"
                    "psi_memory: %s\n"
                    "psi_io: %s\n"
                    "oslab_monitor_overview: %s\n"
                    "tracebench_version: v2-p0\n",
                    uts.sysname,
                    uts.release,
                    uts.version,
                    uts.machine,
                    gcc_version,
                    tb_path_readable("/sys/fs/cgroup/cgroup.controllers") ? "present" : "missing",
                    tb_path_readable("/proc/pressure/cpu") ? "present" : "missing",
                    tb_path_readable("/proc/pressure/memory") ? "present" : "missing",
                    tb_path_readable("/proc/pressure/io") ? "present" : "missing",
                    tb_path_readable("/proc/oslab_monitor/overview") ? "present" : "missing") != 0) {
        return -1;
    }

    return tb_write_text_file(path, text);
}

int tb_write_summary_file(const TbConfig *config, const char *csv_path)
{
    TbCsvSummary summary;
    char path[TB_PATH_LEN];
    char text[TB_REPORT_LINE_LEN];
    int used = 0;

    if (read_csv_summary(csv_path, &summary) != 0) {
        return -1;
    }
    if (tb_join_path(path, sizeof(path), config->output_path, "summary.txt") != 0) {
        return -1;
    }

    if (append_text(text, sizeof(text), &used,
                    "profile: %s\n"
                    "duration_sec: %s\n"
                    "sample_interval_sec: %s\n"
                    "samples_csv: %s\n"
                    "sample_rows: %d\n"
                    "cgroup_enabled: %s\n"
                    "oslab_monitor_available: %s\n",
                    summary.profile,
                    summary.duration_sec,
                    summary.sample_interval_sec,
                    csv_path,
                    summary.sample_rows,
                    summary.cgroup_enabled,
                    summary.oslab_available ? "true" : "false") != 0) {
        return -1;
    }
    if (config->no_cgroup &&
        append_text(text, sizeof(text), &used,
                    "warning: --no-cgroup 为低权限模式，当前摘要不包含 cgroup 指标。\n") != 0) {
        return -1;
    }
    if (append_text(text, sizeof(text), &used, "\nmetrics:\n") != 0) {
        return -1;
    }
    for (int i = 0; i < TB_SUMMARY_METRIC_COUNT; i++) {
        if (append_metric_summary(text, sizeof(text), &used, &summary.metrics[i]) != 0) {
            return -1;
        }
    }

    return tb_write_text_file(path, text);
}

int tb_generate_markdown_report(const TbConfig *config)
{
    TbCsvSummary summary;
    char csv_path[TB_PATH_LEN];
    char parent[TB_PATH_LEN];
    char text[TB_REPORT_LINE_LEN];
    int used = 0;

    if (tb_join_path(csv_path, sizeof(csv_path), config->input_path, "samples.csv") != 0) {
        return -1;
    }
    if (!tb_path_readable(csv_path)) {
        tb_print_error("report input is missing samples.csv: %s", csv_path);
        return -1;
    }
    if (read_csv_summary(csv_path, &summary) != 0) {
        return -1;
    }
    if (tb_parent_dir(parent, sizeof(parent), config->report_output_path) != 0 ||
        tb_mkdir_p(parent) != 0) {
        return -1;
    }

    if (append_text(text, sizeof(text), &used,
                    "# OSLab TraceBench 运行报告\n\n"
                    "## 1. 运行配置\n\n"
                    "- profile: %s\n"
                    "- duration_sec: %s\n"
                    "- sample_interval_sec: %s\n"
                    "- samples.csv: %s\n"
                    "- sample_rows: %d\n\n"
                    "## 2. 运行环境\n\n"
                    "运行环境元数据记录在输入目录的 `environment.txt` 中。\n\n"
                    "## 3. 采样文件\n\n"
                    "本报告基于 `%s` 生成。\n\n"
                    "## 4. PSI 资源压力摘要\n\n",
                    summary.profile,
                    summary.duration_sec,
                    summary.sample_interval_sec,
                    csv_path,
                    summary.sample_rows,
                    csv_path) != 0) {
        return -1;
    }
    for (int i = 0; i < 3; i++) {
        if (append_metric_summary(text, sizeof(text), &used, &summary.metrics[i]) != 0) {
            return -1;
        }
    }
    if (append_text(text, sizeof(text), &used,
                    "\n## 5. cgroup 指标摘要\n\n"
                    "cgroup_enabled: %s\n\n",
                    summary.cgroup_enabled) != 0) {
        return -1;
    }
    for (int i = 3; i < 8; i++) {
        if (append_metric_summary(text, sizeof(text), &used, &summary.metrics[i]) != 0) {
            return -1;
        }
    }
    if (append_text(text, sizeof(text), &used,
                    "\n## 6. oslab_monitor 对照结果\n\n"
                    "oslab_monitor_available: %s\n\n",
                    summary.oslab_available ? "true" : "false") != 0) {
        return -1;
    }
    for (int i = 8; i < TB_SUMMARY_METRIC_COUNT; i++) {
        if (append_metric_summary(text, sizeof(text), &used, &summary.metrics[i]) != 0) {
            return -1;
        }
    }
    if (append_text(text, sizeof(text), &used,
                    "\n## 7. 运行现象观察\n\n"
                    "可结合 PSI、cgroup 和 oslab_monitor 字段观察资源压力变化。\n\n"
                    "## 8. 局限性\n\n"
                    "本报告仅汇总当前 `samples.csv` 的 first、last、delta 和 max 值；"
                    "更细粒度的趋势分析应使用原始 CSV 或外部绘图工具。\n") != 0) {
        return -1;
    }

    return tb_write_text_file(config->report_output_path, text);
}
