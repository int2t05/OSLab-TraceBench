/*
 * 文件作用：解析 tracebench 的 run、report、cleanup 子命令参数。
 * 设计原因：参数规则是 P0 验收入口，集中校验可以避免后续采样和 workload 模块重复判断。
 */

#include "tracebench.h"

#include <string.h>

static void tb_init_defaults(TbConfig *config)
{
    memset(config, 0, sizeof(*config));
    config->command = TB_CMD_HELP;
    config->profile = TB_PROFILE_CPU;
    config->cpu_workers = 2;
    config->memory_mb = 128;
    config->io_mb = 64;
    snprintf(config->cgroup_name, sizeof(config->cgroup_name), "%s", "oslab_tracebench");
}

static int next_value(int argc, char **argv, int *index, const char *option, const char **value)
{
    if (*index + 1 >= argc) {
        tb_print_error("%s requires a value", option);
        return -1;
    }
    *index += 1;
    *value = argv[*index];
    return 0;
}

static int copy_checked(char *dest, int dest_size, const char *value, const char *name)
{
    if ((int)strlen(value) >= dest_size) {
        tb_print_error("%s is too long", name);
        return -1;
    }
    snprintf(dest, (size_t)dest_size, "%s", value);
    return 0;
}

static int parse_profile(const char *text, TbProfile *profile)
{
    if (strcmp(text, "cpu") == 0) {
        *profile = TB_PROFILE_CPU;
        return 0;
    }
    if (strcmp(text, "memory") == 0) {
        *profile = TB_PROFILE_MEMORY;
        return 0;
    }
    if (strcmp(text, "io") == 0) {
        *profile = TB_PROFILE_IO;
        return 0;
    }
    tb_print_error("--profile must be one of cpu, memory, io");
    return -1;
}

static int parse_run_args(int argc, char **argv, TbConfig *config)
{
    int has_profile = 0;
    int has_duration = 0;
    int has_interval = 0;
    int has_output = 0;

    config->command = TB_CMD_RUN;

    for (int i = 2; i < argc; i++) {
        const char *value = NULL;

        if (strcmp(argv[i], "--profile") == 0) {
            if (next_value(argc, argv, &i, argv[i], &value) != 0) {
                return -1;
            }
            if (parse_profile(value, &config->profile) != 0) {
                return -1;
            }
            has_profile = 1;
        } else if (strcmp(argv[i], "--duration") == 0) {
            if (next_value(argc, argv, &i, argv[i], &value) != 0) {
                return -1;
            }
            if (tb_parse_positive_int(value, "--duration", &config->duration_sec) != 0) {
                return -1;
            }
            has_duration = 1;
        } else if (strcmp(argv[i], "--sample-interval") == 0) {
            if (next_value(argc, argv, &i, argv[i], &value) != 0) {
                return -1;
            }
            if (tb_parse_positive_int(value, "--sample-interval", &config->sample_interval_sec) != 0) {
                return -1;
            }
            has_interval = 1;
        } else if (strcmp(argv[i], "--output") == 0) {
            if (next_value(argc, argv, &i, argv[i], &value) != 0) {
                return -1;
            }
            if (copy_checked(config->output_path, TB_PATH_LEN, value, "--output") != 0) {
                return -1;
            }
            has_output = 1;
        } else if (strcmp(argv[i], "--cpu-workers") == 0) {
            if (next_value(argc, argv, &i, argv[i], &value) != 0) {
                return -1;
            }
            if (tb_parse_positive_int(value, "--cpu-workers", &config->cpu_workers) != 0) {
                return -1;
            }
        } else if (strcmp(argv[i], "--memory-mb") == 0) {
            if (next_value(argc, argv, &i, argv[i], &value) != 0) {
                return -1;
            }
            if (tb_parse_positive_int(value, "--memory-mb", &config->memory_mb) != 0) {
                return -1;
            }
        } else if (strcmp(argv[i], "--io-mb") == 0) {
            if (next_value(argc, argv, &i, argv[i], &value) != 0) {
                return -1;
            }
            if (tb_parse_positive_int(value, "--io-mb", &config->io_mb) != 0) {
                return -1;
            }
        } else if (strcmp(argv[i], "--cgroup-name") == 0) {
            if (next_value(argc, argv, &i, argv[i], &value) != 0) {
                return -1;
            }
            if (!tb_is_valid_cgroup_name(value)) {
                tb_print_error("--cgroup-name may only contain letters, digits, '_' and '-'");
                return -1;
            }
            if (copy_checked(config->cgroup_name, TB_NAME_LEN, value, "--cgroup-name") != 0) {
                return -1;
            }
        } else if (strcmp(argv[i], "--no-cgroup") == 0) {
            config->no_cgroup = 1;
        } else if (strcmp(argv[i], "--with-oslab-monitor") == 0) {
            config->with_oslab_monitor = 1;
        } else {
            tb_print_error("unknown run option: %s", argv[i]);
            return -1;
        }
    }

    if (!has_profile) {
        tb_print_error("run requires --profile");
        return -1;
    }
    if (!has_duration) {
        tb_print_error("run requires --duration");
        return -1;
    }
    if (!has_interval) {
        tb_print_error("run requires --sample-interval");
        return -1;
    }
    if (!has_output) {
        tb_print_error("run requires --output");
        return -1;
    }
    if (config->sample_interval_sec > config->duration_sec) {
        tb_print_error("--sample-interval must be positive and no greater than duration");
        return -1;
    }

    return 0;
}

static int parse_report_args(int argc, char **argv, TbConfig *config)
{
    int has_input = 0;
    int has_output = 0;

    config->command = TB_CMD_REPORT;

    for (int i = 2; i < argc; i++) {
        const char *value = NULL;

        if (strcmp(argv[i], "--input") == 0) {
            if (next_value(argc, argv, &i, argv[i], &value) != 0) {
                return -1;
            }
            if (copy_checked(config->input_path, TB_PATH_LEN, value, "--input") != 0) {
                return -1;
            }
            has_input = 1;
        } else if (strcmp(argv[i], "--output") == 0) {
            if (next_value(argc, argv, &i, argv[i], &value) != 0) {
                return -1;
            }
            if (copy_checked(config->report_output_path, TB_PATH_LEN, value, "--output") != 0) {
                return -1;
            }
            has_output = 1;
        } else {
            tb_print_error("unknown report option: %s", argv[i]);
            return -1;
        }
    }

    if (!has_input) {
        tb_print_error("report requires --input");
        return -1;
    }
    if (!has_output) {
        tb_print_error("report requires --output");
        return -1;
    }

    return 0;
}

static int parse_cleanup_args(int argc, char **argv, TbConfig *config)
{
    config->command = TB_CMD_CLEANUP;

    for (int i = 2; i < argc; i++) {
        const char *value = NULL;

        if (strcmp(argv[i], "--cgroup-name") == 0) {
            if (next_value(argc, argv, &i, argv[i], &value) != 0) {
                return -1;
            }
            if (!tb_is_valid_cgroup_name(value)) {
                tb_print_error("--cgroup-name may only contain letters, digits, '_' and '-'");
                return -1;
            }
            if (copy_checked(config->cgroup_name, TB_NAME_LEN, value, "--cgroup-name") != 0) {
                return -1;
            }
        } else {
            tb_print_error("unknown cleanup option: %s", argv[i]);
            return -1;
        }
    }

    return 0;
}

int tb_parse_args(int argc, char **argv, TbConfig *config)
{
    tb_init_defaults(config);

    if (argc == 1) {
        config->command = TB_CMD_HELP;
        return 0;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        config->command = TB_CMD_HELP;
        return 0;
    }
    if (strcmp(argv[1], "run") == 0) {
        return parse_run_args(argc, argv, config);
    }
    if (strcmp(argv[1], "report") == 0) {
        return parse_report_args(argc, argv, config);
    }
    if (strcmp(argv[1], "cleanup") == 0) {
        return parse_cleanup_args(argc, argv, config);
    }

    tb_print_error("unknown command: %s", argv[1]);
    return -1;
}
