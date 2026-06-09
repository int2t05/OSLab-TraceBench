/*
 * 文件作用：保留 TraceBench PSI、cgroup 和 oslab_monitor 采样模块。
 * 设计原因：采样与 CLI 和 workload 解耦，后续任务可以在这里逐步实现稳定 CSV 输出。
 */

#include "tracebench.h"

int tb_read_psi_snapshot(TbPsiSnapshot *snapshot)
{
    (void)snapshot;
    tb_print_error("PSI sampling is not implemented yet");
    return -1;
}

int tb_read_oslab_snapshot(TbOslabSnapshot *snapshot)
{
    (void)snapshot;
    tb_print_error("oslab_monitor sampling is not implemented yet");
    return -1;
}

int tb_write_csv_header(FILE *out)
{
    (void)out;
    tb_print_error("CSV output is not implemented yet");
    return -1;
}

int tb_write_csv_sample(FILE *out, const TbConfig *config, const TbCgroup *cgroup, const TbSample *sample)
{
    (void)out;
    (void)config;
    (void)cgroup;
    (void)sample;
    tb_print_error("CSV output is not implemented yet");
    return -1;
}
