/*
 * 文件作用：实现 TraceBench run/report/cleanup 当前阶段的命令边界。
 * 设计原因：PLANv2 后续会在本文件补齐 summary 和 Markdown 报告生成；
 * 当前阶段只让 CLI 参数测试先通过，未实现的执行命令明确返回 error。
 */

#include "tracebench.h"

int tb_run_command(const TbConfig *config, int argc, char **argv)
{
    (void)config;
    (void)argc;
    (void)argv;
    tb_print_error("run is not implemented yet");
    return 1;
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
    (void)config;
    (void)argc;
    (void)argv;
    tb_print_error("command.txt output is not implemented yet");
    return -1;
}

int tb_write_environment_file(const TbConfig *config)
{
    (void)config;
    tb_print_error("environment.txt output is not implemented yet");
    return -1;
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
