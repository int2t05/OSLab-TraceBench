/*
 * 文件作用：实现 TraceBench run/report/cleanup 当前阶段的命令边界。
 * 设计原因：PLANv2 后续会在本文件补齐 summary 和 Markdown 报告生成；
 * 当前阶段只让 CLI 参数测试先通过，未实现的执行命令明确返回 error。
 */

#include "tracebench.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

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

int tb_run_command(const TbConfig *config, int argc, char **argv)
{
    TbCgroup cgroup;

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

    {
        char csv_path[TB_PATH_LEN];
        FILE *csv;
        TbSample sample;

        memset(&sample, 0, sizeof(sample));
        sample.sample_index = 0;
        sample.elapsed_ms = 0;
        sample.profile = config->profile;

        if (tb_read_psi_snapshot(&sample.psi) != 0) {
            tb_cgroup_remove_run(&cgroup);
            return 1;
        }
        if (tb_cgroup_read_stats(&cgroup, &sample.cgroup) != 0) {
            tb_cgroup_remove_run(&cgroup);
            return 1;
        }
        if (tb_join_path(csv_path, sizeof(csv_path), config->output_path, "samples.csv") != 0) {
            tb_cgroup_remove_run(&cgroup);
            return 1;
        }
        csv = fopen(csv_path, "w");
        if (csv == NULL) {
            tb_print_error("failed to open %s for writing", csv_path);
            tb_cgroup_remove_run(&cgroup);
            return 1;
        }
        if (tb_write_csv_header(csv) != 0 ||
            tb_write_csv_sample(csv, config, &cgroup, &sample) != 0) {
            fclose(csv);
            tb_cgroup_remove_run(&cgroup);
            return 1;
        }
        if (fclose(csv) != 0) {
            tb_print_error("failed to close %s", csv_path);
            tb_cgroup_remove_run(&cgroup);
            return 1;
        }
    }

    if (tb_cgroup_remove_run(&cgroup) != 0) {
        return 1;
    }

    fprintf(stderr, "tracebench: sampling and workload are not implemented yet\n");
    return 0;
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
