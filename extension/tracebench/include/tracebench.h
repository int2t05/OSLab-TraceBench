/*
 * 文件作用：声明 TraceBench v2 用户态工具的模块内共享类型和函数。
 * 设计原因：TraceBench 由多个 C 文件协作完成 CLI、采样、cgroup 和报告生成；
 * 将共享契约集中在本头文件中，可以保持 extension/tracebench 内部边界清晰。
 */

#ifndef TRACEBENCH_H
#define TRACEBENCH_H

#include <stdio.h>

#define TB_NAME_LEN 64
#define TB_PATH_LEN 512
#define TB_LINE_LEN 1024
#define TB_VALUE_LEN 128

typedef enum {
    TB_CMD_HELP,
    TB_CMD_RUN,
    TB_CMD_REPORT,
    TB_CMD_CLEANUP
} TbCommand;

typedef enum {
    TB_PROFILE_CPU,
    TB_PROFILE_MEMORY,
    TB_PROFILE_IO
} TbProfile;

typedef struct {
    TbCommand command;
    TbProfile profile;
    int duration_sec;
    int sample_interval_sec;
    int cpu_workers;
    int memory_mb;
    int io_mb;
    int no_cgroup;
    int with_oslab_monitor;
    char cgroup_name[TB_NAME_LEN];
    char output_path[TB_PATH_LEN];
    char input_path[TB_PATH_LEN];
    char report_output_path[TB_PATH_LEN];
} TbConfig;

typedef struct {
    int enabled;
    char root_path[TB_PATH_LEN];
    char profile_path[TB_PATH_LEN];
    char run_path[TB_PATH_LEN];
    char run_id[TB_NAME_LEN];
} TbCgroup;

typedef struct {
    int present;
    double avg10;
    double avg60;
    double avg300;
    unsigned long long total;
} TbPsiLine;

typedef struct {
    TbPsiLine some;
    TbPsiLine full;
} TbPsiResource;

typedef struct {
    TbPsiResource cpu;
    TbPsiResource memory;
    TbPsiResource io;
} TbPsiSnapshot;

typedef struct {
    int enabled;
    unsigned long long cpu_usage_usec;
    unsigned long long cpu_user_usec;
    unsigned long long cpu_system_usec;
    unsigned long long cpu_nr_periods;
    unsigned long long cpu_nr_throttled;
    unsigned long long cpu_throttled_usec;
    unsigned long long memory_current;
    unsigned long long memory_events_low;
    unsigned long long memory_events_high;
    unsigned long long memory_events_max;
    unsigned long long memory_events_oom;
    unsigned long long memory_events_oom_kill;
    unsigned long long memory_events_oom_group_kill;
    int has_cpu_user_usec;
    int has_cpu_system_usec;
    int has_cpu_nr_periods;
    int has_cpu_nr_throttled;
    int has_cpu_throttled_usec;
    int has_oom_kill;
    int has_oom_group_kill;
} TbCgroupStats;

typedef struct {
    int available;
    int total_tasks_present;
    int running_tasks_present;
    int sleeping_tasks_present;
    int mem_free_kb_present;
    int mem_available_kb_present;
    unsigned long long total_tasks;
    unsigned long long running_tasks;
    unsigned long long sleeping_tasks;
    unsigned long long mem_free_kb;
    unsigned long long mem_available_kb;
} TbOslabSnapshot;

typedef struct {
    int sample_index;
    long long elapsed_ms;
    TbProfile profile;
    TbPsiSnapshot psi;
    TbCgroupStats cgroup;
    TbOslabSnapshot oslab;
} TbSample;

int tb_parse_args(int argc, char **argv, TbConfig *config);
void tb_print_help(const char *argv0);

int tb_run_command(const TbConfig *config, int argc, char **argv);
int tb_report_command(const TbConfig *config);
int tb_cleanup_command(const TbConfig *config);

int tb_cgroup_init(const TbConfig *config, TbCgroup *cgroup);
int tb_cgroup_create(TbCgroup *cgroup);
int tb_cgroup_add_pid(const TbCgroup *cgroup, int pid);
int tb_cgroup_read_stats(const TbCgroup *cgroup, TbCgroupStats *stats);
int tb_cgroup_remove_run(const TbCgroup *cgroup);
int tb_cgroup_cleanup_all(const char *cgroup_name);

int tb_run_workload_child(const TbConfig *config, int start_fd);

int tb_read_psi_snapshot(TbPsiSnapshot *snapshot);
int tb_read_oslab_snapshot(TbOslabSnapshot *snapshot);
int tb_write_csv_header(FILE *out);
int tb_write_csv_sample(FILE *out, const TbConfig *config, const TbCgroup *cgroup, const TbSample *sample);

int tb_write_command_file(const TbConfig *config, int argc, char **argv);
int tb_write_environment_file(const TbConfig *config);
int tb_write_summary_file(const TbConfig *config, const char *csv_path);
int tb_generate_markdown_report(const TbConfig *config);

int tb_mkdir_p(const char *path);
int tb_read_text_file(const char *path, char *buffer, int buffer_size);
int tb_write_text_file(const char *path, const char *text);
long long tb_now_millis(void);
void tb_print_error(const char *fmt, ...);
int tb_parse_positive_int(const char *text, const char *name, int *value);
int tb_is_valid_cgroup_name(const char *name);
const char *tb_profile_name(TbProfile profile);

#endif
