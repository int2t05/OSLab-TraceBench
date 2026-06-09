/*
 * 文件作用：实现 tracebench 命令行入口和子命令分发。
 * 设计原因：入口文件只处理顶层控制流，避免把参数解析、cgroup、采样或报告细节混入 main。
 */

#include "tracebench.h"

int main(int argc, char **argv)
{
    TbConfig config;

    if (tb_parse_args(argc, argv, &config) != 0) {
        return 1;
    }

    switch (config.command) {
    case TB_CMD_HELP:
        tb_print_help(argv[0]);
        return 0;
    case TB_CMD_RUN:
        return tb_run_command(&config, argc, argv);
    case TB_CMD_REPORT:
        return tb_report_command(&config);
    case TB_CMD_CLEANUP:
        return tb_cleanup_command(&config);
    }

    tb_print_error("unknown command");
    return 1;
}
