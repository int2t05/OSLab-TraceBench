# OSLab TraceBench v2 实验报告模板

本文档用于整理 TraceBench v2 资源压力实验报告。P0 已实现 `extension/tracebench/tracebench`，可通过 `tracebench run` 生成采样数据，并通过 `tracebench report` 生成与本模板结构一致的 Markdown 报告。

报告数据来源：

- `samples.csv`：由 `tracebench run` 生成，保存 PSI、cgroup v2 和 oslab_monitor 对照采样。
- `summary.txt`：由 `tracebench run` 生成，保存关键指标摘要。
- `report.md`：由 `tracebench report` 根据 `samples.csv` 生成。
- `environment.txt`：由 `tracebench run` 生成，保存系统环境。
- `command.txt`：由 `tracebench run` 生成，保存本次实验命令和参数。

## 1. 实验配置

报告中记录：

- profile：`cpu`、`memory` 或 `io`。
- duration：实验持续秒数。
- sample interval：采样间隔秒数。
- output：实验输出目录。
- cgroup name：实验 cgroup 名称。
- cpu-workers：CPU profile 使用的 worker 数。
- memory-mb：memory profile 使用的内存压力大小。
- io-mb：io profile 使用的写入大小。
- no-cgroup：是否启用低权限演示模式。
- with-oslab-monitor：是否要求 `/proc/oslab_monitor/overview` 必须可用。

## 2. 运行环境

报告中记录：

- Linux 发行版。
- `uname -a` 输出。
- gcc 版本。
- make 版本。
- cgroup v2 是否挂载。
- `/proc/pressure/cpu`、`/proc/pressure/memory`、`/proc/pressure/io` 是否存在且可读。
- `/proc/oslab_monitor/overview` 是否存在。
- 是否以 root 权限运行默认 P0 实验。

## 3. 采样文件

报告中记录：

- `samples.csv` 路径。
- CSV 表头是否包含 PSI 字段。
- CSV 表头是否包含 cgroup 字段。
- CSV 表头是否包含 oslab_monitor 字段。
- 采样行数。
- 每行字段数量是否一致。
- `command.txt`、`environment.txt`、`summary.txt`、`report.md` 是否生成。

## 4. PSI 资源压力摘要

报告中记录以下字段的 first、last、delta、max 摘要：

- `cpu_some_total`
- `memory_some_total`
- `io_some_total`
- `cpu_some_avg10`
- `memory_some_avg10`
- `io_some_avg10`

分析口径：

- CPU profile 重点观察 CPU PSI 是否随 CPU worker 运行产生变化。
- memory profile 重点观察 memory PSI 是否随内存申请和访问产生变化。
- io profile 重点观察 I/O PSI 是否随临时文件写入和 `fsync` 产生变化。
- 如果某个 PSI `full` 行不存在，报告中记录该字段未采集，并说明不影响 P0 的 `some` 指标分析。

## 5. cgroup 指标摘要

报告中记录以下字段的 first、last、delta、max 摘要：

- `cgroup_cpu_usage_usec`
- `cgroup_cpu_user_usec`
- `cgroup_cpu_system_usec`
- `cgroup_memory_current`
- `cgroup_memory_events_low`
- `cgroup_memory_events_high`
- `cgroup_memory_events_max`
- `cgroup_memory_events_oom`

分析口径：

- CPU profile 重点观察 `cgroup_cpu_usage_usec` 是否增长。
- memory profile 重点观察 `cgroup_memory_current` 是否接近配置的内存压力大小。
- memory events 中的 `oom`、`max` 等字段用于说明实验是否触发内存压力边界。
- 使用 `--no-cgroup` 时，报告必须说明 cgroup 字段为 `NA`，该运行只属于低权限演示，不满足 P0 完整验收。

## 6. oslab_monitor 对照结果

报告中记录：

- `oslab_monitor_available`。
- `oslab_total_tasks`。
- `oslab_running_tasks`。
- `oslab_sleeping_tasks`。
- `oslab_mem_free_kb`。
- `oslab_mem_available_kb`。

分析口径：

- 如果 `oslab_monitor_available=true`，说明已加载 `oslab_monitor.ko`，可将自研 `/proc/oslab_monitor/overview` 输出与 PSI/cgroup 采样放在同一次实验中对照。
- 如果 `oslab_monitor_available=false`，说明未加载扩展内核模块；默认模式下 TraceBench 仍可完成 P0 采样。
- 如果使用 `--with-oslab-monitor`，报告必须记录该接口存在并可读，否则该次实验应失败。

## 7. 实验现象分析

报告中至少分析：

- 当前 profile 制造的主要资源压力类型。
- PSI 指标变化与 workload 行为之间的关系。
- cgroup 指标变化与 workload 归组之间的关系。
- oslab_monitor 对照字段能说明的系统状态变化。
- 实验结果与操作系统课程中的调度、内存管理或 I/O 等概念之间的联系。

## 8. 局限性

报告中记录：

- 实验结果受 VM CPU 核数、内存大小、磁盘性能和后台进程影响。
- PSI 是系统或 cgroup 层面的压力指标，不等同于单个进程的完整性能剖析。
- P0 默认不设置 `cpu.max`、`memory.max` 或 `memory.high`，重点是归组采样，不是强资源限制。
- I/O profile 的压力效果受文件系统缓存和虚拟磁盘实现影响。
- 在 WSL2 或未启用 cgroup v2、PSI 的环境中不能宣称 P0 验收通过。

## 9. 可选增强说明

报告中记录 P1/P2 状态：

- tracefs 是否启用；若未启用，记录原因。
- bpftrace 是否启用；若未启用，记录原因。
- sched_ext 是否尝试；若未尝试，记录其为 P2 挑战项。
- P1/P2 不支持时，不影响 P0 的 PSI、cgroup 和 oslab_monitor 数据采集结论。
