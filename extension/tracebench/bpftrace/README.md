# TraceBench bpftrace 预留目录

本目录用于 TraceBench v2 P1 可选 bpftrace 增强，不属于 P0 默认验收范围。

P0 的 `Makefile`、`tracebench` 可执行文件和集成测试不依赖 bpftrace；未安装 bpftrace 时，P0 不应失败。

后续如环境支持，可在本目录添加：

- `sched_latency.bt`
- `syscall_count.bt`

这些脚本必须作为可降级增强处理，不能成为 P0 运行前提。
