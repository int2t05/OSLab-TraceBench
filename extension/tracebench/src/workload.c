/*
 * 文件作用：保留 TraceBench workload 子进程模块的编译单元。
 * 设计原因：CPU、memory、I/O 压力生成属于独立职责，后续任务会在此实现，
 * 当前阶段仅提供可链接的函数边界。
 */

#include "tracebench.h"

int tb_run_workload_child(const TbConfig *config, int start_fd)
{
    (void)config;
    (void)start_fd;
    tb_print_error("workload support is not implemented yet");
    return -1;
}
