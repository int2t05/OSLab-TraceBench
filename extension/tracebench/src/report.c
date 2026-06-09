/*
 * 文件作用：实现 TraceBench run/report/cleanup 当前阶段的命令边界。
 * 设计原因：PLANv2 后续会在本文件补齐 summary 和 Markdown 报告生成；
 * 当前阶段只让 CLI 参数测试先通过，未实现的执行命令明确返回 error。
 */

#include "tracebench.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t run_interrupted = 0;
static pid_t active_child_pid = -1;

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
    sigaction(SIGINT, &old_int, NULL);
    sigaction(SIGTERM, &old_term, NULL);
    return result;
}

int tb_report_command(const TbConfig *config)
{
    (void)config;
    tb_print_error("report is not implemented yet");
    return 1;
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
    (void)config;
    (void)csv_path;
    tb_print_error("summary.txt output is not implemented yet");
    return -1;
}

int tb_generate_markdown_report(const TbConfig *config)
{
    (void)config;
    tb_print_error("markdown report is not implemented yet");
    return -1;
}
