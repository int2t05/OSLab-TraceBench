/*
 * 文件作用：保留 TraceBench cgroup v2 管理模块的编译单元。
 * 设计原因：PLANv2 要求 cgroup 逻辑独立于 CLI；当前阶段只提供占位函数，
 * 后续任务会在同一文件中实现检测、创建、采样和清理。
 */

#include "tracebench.h"

int tb_cgroup_init(const TbConfig *config, TbCgroup *cgroup)
{
    (void)config;
    (void)cgroup;
    tb_print_error("cgroup support is not implemented yet");
    return -1;
}

int tb_cgroup_create(TbCgroup *cgroup)
{
    (void)cgroup;
    tb_print_error("cgroup creation is not implemented yet");
    return -1;
}

int tb_cgroup_add_pid(const TbCgroup *cgroup, int pid)
{
    (void)cgroup;
    (void)pid;
    tb_print_error("cgroup pid attachment is not implemented yet");
    return -1;
}

int tb_cgroup_read_stats(const TbCgroup *cgroup, TbCgroupStats *stats)
{
    (void)cgroup;
    (void)stats;
    tb_print_error("cgroup stats are not implemented yet");
    return -1;
}

int tb_cgroup_remove_run(const TbCgroup *cgroup)
{
    (void)cgroup;
    tb_print_error("cgroup run cleanup is not implemented yet");
    return -1;
}

int tb_cgroup_cleanup_all(const char *cgroup_name)
{
    (void)cgroup_name;
    tb_print_error("cleanup is not implemented yet");
    return -1;
}
