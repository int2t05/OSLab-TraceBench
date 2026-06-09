# 能力清单

本文档汇总 OSLab TraceBench 已实现的用户可见能力。项目提供确定性的操作系统机制模型、Linux `/proc` 观测模块，以及用于 Linux 运行态分析的资源压力采样工具。

## 验证状态

- `basic/` 用户态模块通过 `bash tests/run_all.sh` 验证。
- `extension/oslab_monitor/` 在 Ubuntu VM 中通过 `bash tests/test_oslab_monitor.sh` 验证。
- `extension/tracebench/` 在 Ubuntu VM 中通过 `sudo bash tests/test_tracebench.sh` 验证。

## 1. 用户态 OS 模型

`basic/` 目录包含四个相互独立的 C 命令行程序。每个模块都有自己的 `Makefile`、源码目录、头文件、固定输入文件和验证脚本。

### 1.1 处理机调度

路径：`basic/scheduler/`

可执行文件：`scheduler`

能力：

- FCFS 调度。
- 非抢占式 SJF 调度。
- 时间片轮转 RR 调度。
- 非抢占式优先级调度。
- 稳定输出时间线、单进程指标、平均等待时间、平均周转时间和平均带权周转时间。

命令：

```bash
./scheduler --algorithm fcfs < tests/sample.txt
./scheduler --algorithm sjf < tests/sample.txt
./scheduler --algorithm rr < tests/sample.txt
./scheduler --algorithm priority < tests/sample.txt
```

### 1.2 内存管理

路径：`basic/memory/`

可执行文件：`memory`

能力：

- 动态分区首次适应和最佳适应分配。
- 分区分配、释放和相邻空闲分区合并。
- FIFO 和 LRU 页面置换。
- 稳定输出分区状态、页框状态、缺页次数和缺页率。

命令：

```bash
./memory --mode partition --algorithm ff < tests/partition.txt
./memory --mode partition --algorithm bf < tests/partition.txt
./memory --mode paging --algorithm fifo < tests/pages.txt
./memory --mode paging --algorithm lru < tests/pages.txt
```

### 1.3 同步工作负载

路径：`basic/sync/`

可执行文件：`sync`

能力：

- 生产者-消费者同步。
- 写者优先的读者-写者同步。
- 带死锁规避的哲学家进餐同步。
- 基于 pthread 的并发执行和稳定汇总计数。

命令：

```bash
./sync --problem producer_consumer --producers 2 --consumers 2 --buffer-size 4 --count 10
./sync --problem readers_writers --readers 3 --writers 2 --count 5
./sync --problem dining_philosophers --count 3
```

### 1.4 内存型文件系统

路径：`basic/filesystem/`

可执行文件：`filesystem`

能力：

- 内存型虚拟磁盘。
- 多级目录树。
- 块位图分配。
- 文件创建、覆盖写、读取和删除。
- 目录列表和文件系统统计。

命令：

```bash
./filesystem < tests/fs_commands.txt
```

支持操作：

- `mkfs DISK_SIZE BLOCK_SIZE`
- `mkdir PATH`
- `create PATH`
- `write PATH CONTENT`
- `read PATH`
- `ls PATH`
- `delete PATH`
- `stat`

## 2. Linux 运行态观测

`extension/oslab_monitor/` 模块通过 `/proc/oslab_monitor/` 导出 Linux 进程和内存状态，并提供一个轻量用户态 CLI。

### 2.1 内核模块

路径：`extension/oslab_monitor/kernel/`

模块：`oslab_monitor.ko`

能力：

- 加载时创建 `/proc/oslab_monitor/`。
- 导出 `overview`、`tasks` 和 `pid` 三个 proc 节点。
- 卸载时清理全部 proc 节点。
- 使用稳定文本字段，便于脚本解析。

命令：

```bash
make
sudo insmod oslab_monitor.ko
sudo rmmod oslab_monitor
```

### 2.2 `/proc/oslab_monitor/overview`

能力：

- 模块名和内核版本。
- 进程状态计数。
- 系统内存总量和可用量。
- 读取时的 jiffies。

命令：

```bash
cat /proc/oslab_monitor/overview
```

### 2.3 `/proc/oslab_monitor/tasks`

能力：

- 遍历进程列表。
- 输出 PID、命令名、状态、调度策略、优先级、nice 值、线程数、RSS 和缺页字段。

命令：

```bash
cat /proc/oslab_monitor/tasks
```

### 2.4 `/proc/oslab_monitor/pid`

能力：

- root 可写目标 PID。
- 输出指定 PID 的进程详情。
- 进程不存在时给出明确结果。

权限：

- `overview/tasks = 0444`
- `pid = 0644`

命令：

```bash
echo 1 | sudo tee /proc/oslab_monitor/pid
cat /proc/oslab_monitor/pid
```

### 2.5 `oslabctl`

路径：`extension/oslab_monitor/user/`

可执行文件：`oslabctl`

能力：

- `oslabctl --help`
- `oslabctl overview`
- `oslabctl tasks`
- `sudo oslabctl pid <PID>`
- 在模块未加载、参数非法或权限不足时返回清晰错误。

## 3. TraceBench

TraceBench 位于 `extension/tracebench/`，是用于可控资源压力和运行态采样的 C 用户态工具。

### 3.1 命令接口

可执行文件：

```text
extension/tracebench/tracebench
```

命令：

```bash
./tracebench --help
sudo ./tracebench run --profile cpu --duration 3 --sample-interval 1 --cpu-workers 2 --output output/cpu_run
sudo ./tracebench run --profile memory --duration 3 --sample-interval 1 --memory-mb 64 --output output/memory_run
sudo ./tracebench run --profile io --duration 3 --sample-interval 1 --io-mb 16 --output output/io_run
./tracebench report --input output/cpu_run --output output/cpu_run/report.md
sudo ./tracebench cleanup
```

### 3.2 运行能力

- CPU、内存和 I/O 压力 profile。
- cgroup v2 运行分组。
- 从 `/proc/pressure/cpu`、`/proc/pressure/memory` 和 `/proc/pressure/io` 采样 PSI。
- 从 `cpu.stat`、`memory.current` 和 `memory.events` 采样 cgroup 指标。
- 可选读取 `/proc/oslab_monitor/overview` 做对照。
- 通过 `samples.csv` 输出 CSV 数据。
- 通过 `command.txt` 和 `environment.txt` 记录运行元数据。
- 通过 `summary.txt` 输出摘要。
- 通过 `tracebench report` 生成 Markdown 报告。
- 通过 `tracebench cleanup` 清理运行态资源。

## 4. 非目标

本仓库不包含：

- GUI 或 Web 前端。
- Linux 内核源码修改。
- 宿主机调度器替换。
- 生产级文件系统实现。
- 对 `stress-ng`、`fio`、`perf`、`bpftrace`、Prometheus 或 Grafana 的强制依赖。
- Windows 原生内核模块或 cgroup 工作流验证。
