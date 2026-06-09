/*
 * 文件作用：保留 TraceBench cgroup v2 管理模块的编译单元。
 * 设计原因：PLANv2 要求 cgroup 逻辑独立于 CLI；当前阶段只提供占位函数，
 * 后续任务会在同一文件中实现检测、创建、采样和清理。
 */

#include "tracebench.h"

#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static int path_join_raw(char *dest, int size, const char *left, const char *right)
{
    int written = snprintf(dest, (size_t)size, "%s/%s", left, right);
    if (written < 0 || written >= size) {
        tb_print_error("cgroup path is too long");
        return -1;
    }
    return 0;
}

static int ensure_dir(const char *path)
{
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
        tb_print_error("failed to create cgroup %s: %s", path, strerror(errno));
        return -1;
    }
    return 0;
}

static void write_optional_text_file(const char *path, const char *text)
{
    FILE *file = fopen(path, "w");

    if (file == NULL) {
        return;
    }
    (void)fputs(text, file);
    (void)fclose(file);
}

static void enable_child_controllers(const char *path)
{
    char controllers_path[TB_PATH_LEN];
    char subtree_path[TB_PATH_LEN];
    char controllers[TB_LINE_LEN];
    const char *needed[] = {"cpu", "memory", "io"};

    if (tb_join_path(controllers_path, sizeof(controllers_path), path, "cgroup.controllers") != 0 ||
        tb_join_path(subtree_path, sizeof(subtree_path), path, "cgroup.subtree_control") != 0) {
        return;
    }
    if (tb_read_text_file(controllers_path, controllers, sizeof(controllers)) != 0) {
        return;
    }

    for (int i = 0; i < 3; i++) {
        char needle[TB_VALUE_LEN];
        char text[TB_VALUE_LEN];

        snprintf(needle, sizeof(needle), "%s", needed[i]);
        if (strstr(controllers, needle) != NULL) {
            snprintf(text, sizeof(text), "+%s\n", needed[i]);
            write_optional_text_file(subtree_path, text);
        }
    }
}

static void make_run_id(char *dest, int size)
{
    time_t now = time(NULL);
    struct tm tm_now;

    localtime_r(&now, &tm_now);
    snprintf(dest, (size_t)size, "%04d%02d%02d_%02d%02d%02d_%ld",
             tm_now.tm_year + 1900,
             tm_now.tm_mon + 1,
             tm_now.tm_mday,
             tm_now.tm_hour,
             tm_now.tm_min,
             tm_now.tm_sec,
             (long)getpid());
}

int tb_cgroup_init(const TbConfig *config, TbCgroup *cgroup)
{
    memset(cgroup, 0, sizeof(*cgroup));

    if (config->no_cgroup) {
        cgroup->enabled = 0;
        return 0;
    }

    if (!tb_is_root()) {
        tb_print_error("cgroup mode requires root; rerun with sudo or use --no-cgroup for low-permission demo");
        return -1;
    }
    if (!tb_path_readable("/sys/fs/cgroup/cgroup.controllers")) {
        tb_print_error("cgroup v2 is required at /sys/fs/cgroup");
        return -1;
    }

    cgroup->enabled = 1;
    make_run_id(cgroup->run_id, sizeof(cgroup->run_id));
    if (path_join_raw(cgroup->root_path, sizeof(cgroup->root_path),
                      "/sys/fs/cgroup", config->cgroup_name) != 0) {
        return -1;
    }
    if (path_join_raw(cgroup->profile_path, sizeof(cgroup->profile_path),
                      cgroup->root_path, tb_profile_name(config->profile)) != 0) {
        return -1;
    }
    if (path_join_raw(cgroup->run_path, sizeof(cgroup->run_path),
                      cgroup->profile_path, cgroup->run_id) != 0) {
        return -1;
    }

    return 0;
}

int tb_cgroup_create(TbCgroup *cgroup)
{
    if (!cgroup->enabled) {
        return 0;
    }
    if (ensure_dir(cgroup->root_path) != 0) {
        return -1;
    }
    enable_child_controllers(cgroup->root_path);
    if (ensure_dir(cgroup->profile_path) != 0) {
        return -1;
    }
    enable_child_controllers(cgroup->profile_path);
    if (ensure_dir(cgroup->run_path) != 0) {
        return -1;
    }
    return 0;
}

int tb_cgroup_add_pid(const TbCgroup *cgroup, int pid)
{
    char path[TB_PATH_LEN];
    char text[TB_VALUE_LEN];

    if (!cgroup->enabled) {
        return 0;
    }

    if (tb_join_path(path, sizeof(path), cgroup->run_path, "cgroup.procs") != 0) {
        return -1;
    }
    snprintf(text, sizeof(text), "%d\n", pid);
    return tb_write_text_file(path, text);
}

static int parse_key_value_file(const char *path,
                                TbCgroupStats *stats,
                                int is_cpu,
                                int *required_seen)
{
    char text[TB_LINE_LEN * 4];
    char *line;
    char *saveptr = NULL;

    if (tb_read_text_file(path, text, sizeof(text)) != 0) {
        return -1;
    }

    line = strtok_r(text, "\n", &saveptr);
    while (line != NULL) {
        char key[TB_VALUE_LEN];
        unsigned long long value;

        if (sscanf(line, "%127s %llu", key, &value) == 2) {
            if (is_cpu) {
                if (strcmp(key, "usage_usec") == 0) {
                    stats->cpu_usage_usec = value;
                    required_seen[0] = 1;
                } else if (strcmp(key, "user_usec") == 0) {
                    stats->cpu_user_usec = value;
                    stats->has_cpu_user_usec = 1;
                } else if (strcmp(key, "system_usec") == 0) {
                    stats->cpu_system_usec = value;
                    stats->has_cpu_system_usec = 1;
                } else if (strcmp(key, "nr_periods") == 0) {
                    stats->cpu_nr_periods = value;
                    stats->has_cpu_nr_periods = 1;
                } else if (strcmp(key, "nr_throttled") == 0) {
                    stats->cpu_nr_throttled = value;
                    stats->has_cpu_nr_throttled = 1;
                } else if (strcmp(key, "throttled_usec") == 0) {
                    stats->cpu_throttled_usec = value;
                    stats->has_cpu_throttled_usec = 1;
                }
            } else {
                if (strcmp(key, "low") == 0) {
                    stats->memory_events_low = value;
                    required_seen[0] = 1;
                } else if (strcmp(key, "high") == 0) {
                    stats->memory_events_high = value;
                    required_seen[1] = 1;
                } else if (strcmp(key, "max") == 0) {
                    stats->memory_events_max = value;
                    required_seen[2] = 1;
                } else if (strcmp(key, "oom") == 0) {
                    stats->memory_events_oom = value;
                    required_seen[3] = 1;
                } else if (strcmp(key, "oom_kill") == 0) {
                    stats->memory_events_oom_kill = value;
                    stats->has_oom_kill = 1;
                } else if (strcmp(key, "oom_group_kill") == 0) {
                    stats->memory_events_oom_group_kill = value;
                    stats->has_oom_group_kill = 1;
                }
            }
        }
        line = strtok_r(NULL, "\n", &saveptr);
    }

    return 0;
}

int tb_cgroup_read_stats(const TbCgroup *cgroup, TbCgroupStats *stats)
{
    char path[TB_PATH_LEN];
    char text[TB_VALUE_LEN];
    int cpu_seen[1] = {0};
    int memory_events_seen[4] = {0};

    memset(stats, 0, sizeof(*stats));
    if (!cgroup->enabled) {
        stats->enabled = 0;
        return 0;
    }
    stats->enabled = 1;

    if (tb_join_path(path, sizeof(path), cgroup->run_path, "cpu.stat") != 0 ||
        parse_key_value_file(path, stats, 1, cpu_seen) != 0) {
        return -1;
    }
    if (!cpu_seen[0]) {
        tb_print_error("cpu.stat missing usage_usec");
        return -1;
    }

    if (tb_join_path(path, sizeof(path), cgroup->run_path, "memory.current") != 0 ||
        tb_read_text_file(path, text, sizeof(text)) != 0) {
        return -1;
    }
    stats->memory_current = strtoull(text, NULL, 10);

    if (tb_join_path(path, sizeof(path), cgroup->run_path, "memory.events") != 0 ||
        parse_key_value_file(path, stats, 0, memory_events_seen) != 0) {
        return -1;
    }
    for (int i = 0; i < 4; i++) {
        if (!memory_events_seen[i]) {
            tb_print_error("memory.events missing required field");
            return -1;
        }
    }

    return 0;
}

int tb_cgroup_remove_run(const TbCgroup *cgroup)
{
    if (!cgroup->enabled) {
        return 0;
    }

    if (tb_remove_empty_dir(cgroup->run_path) != 0) {
        fprintf(stderr, "warning: could not remove cgroup %s; it may still be busy\n", cgroup->run_path);
        return -1;
    }
    if (tb_remove_empty_dir(cgroup->profile_path) != 0) {
        fprintf(stderr, "warning: cgroup profile directory not removed because it is not empty: %s\n",
                cgroup->profile_path);
    }
    if (tb_remove_empty_dir(cgroup->root_path) != 0) {
        fprintf(stderr, "warning: cgroup root directory not removed because it is not empty: %s\n",
                cgroup->root_path);
    }

    return 0;
}

static int cgroup_has_processes(const char *path)
{
    char procs_path[TB_PATH_LEN];
    char text[TB_VALUE_LEN];

    if (tb_join_path(procs_path, sizeof(procs_path), path, "cgroup.procs") != 0) {
        return 1;
    }
    if (tb_read_text_file(procs_path, text, sizeof(text)) != 0) {
        return 1;
    }

    return text[0] != '\0';
}

static int cleanup_cgroup_children(const char *path)
{
    DIR *dir;
    struct dirent *entry;
    int result = 0;

    dir = opendir(path);
    if (dir == NULL) {
        if (errno == ENOENT) {
            return 0;
        }
        tb_print_error("failed to open cgroup directory %s: %s", path, strerror(errno));
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        char child_path[TB_PATH_LEN];
        struct stat st;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        if (path_join_raw(child_path, sizeof(child_path), path, entry->d_name) != 0) {
            result = -1;
            continue;
        }
        if (stat(child_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
            continue;
        }
        if (cleanup_cgroup_children(child_path) != 0) {
            result = -1;
        }
        if (cgroup_has_processes(child_path)) {
            fprintf(stderr, "warning: cgroup still has processes, not removing: %s\n", child_path);
            result = -1;
            continue;
        }
        if (tb_remove_empty_dir(child_path) != 0 && errno != ENOENT) {
            fprintf(stderr, "warning: could not remove cgroup %s: %s\n", child_path, strerror(errno));
            result = -1;
        }
    }

    closedir(dir);
    return result;
}

static int cleanup_cgroup_root(const char *cgroup_name)
{
    char root_path[TB_PATH_LEN];
    struct stat st;
    int result = 0;

    if (path_join_raw(root_path, sizeof(root_path), "/sys/fs/cgroup", cgroup_name) != 0) {
        return -1;
    }
    if (stat(root_path, &st) != 0) {
        if (errno == ENOENT) {
            return 0;
        }
        tb_print_error("failed to stat cgroup %s: %s", root_path, strerror(errno));
        return -1;
    }
    if (!S_ISDIR(st.st_mode)) {
        tb_print_error("cgroup path is not a directory: %s", root_path);
        return -1;
    }

    if (cleanup_cgroup_children(root_path) != 0) {
        result = -1;
    }
    if (cgroup_has_processes(root_path)) {
        fprintf(stderr, "warning: cgroup still has processes, not removing: %s\n", root_path);
        return -1;
    }
    if (tb_remove_empty_dir(root_path) != 0 && errno != ENOENT) {
        fprintf(stderr, "warning: could not remove cgroup %s: %s\n", root_path, strerror(errno));
        result = -1;
    }

    return result;
}

static int cleanup_io_temp_files(void)
{
    DIR *dir;
    struct dirent *entry;
    int result = 0;

    dir = opendir("output");
    if (dir == NULL) {
        if (errno == ENOENT) {
            return 0;
        }
        tb_print_error("failed to open output directory: %s", strerror(errno));
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        char temp_path[TB_PATH_LEN];
        char child_path[TB_PATH_LEN];
        struct stat st;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        if (path_join_raw(child_path, sizeof(child_path), "output", entry->d_name) != 0) {
            result = -1;
            continue;
        }
        if (stat(child_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
            continue;
        }
        if (path_join_raw(temp_path, sizeof(temp_path), child_path, "tracebench_io.tmp") != 0) {
            result = -1;
            continue;
        }
        if (unlink(temp_path) != 0 && errno != ENOENT) {
            tb_print_error("failed to remove %s: %s", temp_path, strerror(errno));
            result = -1;
        }
    }

    closedir(dir);
    return result;
}

int tb_cgroup_cleanup_all(const char *cgroup_name)
{
    int result = 0;

    if (!tb_is_root()) {
        tb_print_error("cleanup requires root; rerun with sudo");
        return -1;
    }
    if (!tb_is_valid_cgroup_name(cgroup_name)) {
        tb_print_error("invalid cgroup name for cleanup");
        return -1;
    }

    if (cleanup_cgroup_root(cgroup_name) != 0) {
        result = -1;
    }
    if (cleanup_io_temp_files() != 0) {
        result = -1;
    }

    return result;
}
