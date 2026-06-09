# TECH v2：TraceBench 技术设计

## 1. 设计目标

TraceBench 是一个单一 C 命令行程序，用于生成可控 Linux 资源压力并采样运行态信号。它位于 `extension/tracebench/`，独立于基础 OS 模型和 `oslab_monitor` 内核模块。

设计目标：

- 最小依赖：C、Makefile、Bash 和 Linux 系统接口。
- 稳定文本和 CSV 输出。
- 明确的 cgroup v2 权限行为。
- 安全的清理边界。
- 可复现的命令、环境、采样、摘要和报告材料。

## 2. 架构

```text
tracebench
├── args.c       命令行解析和校验
├── cgroup.c     cgroup v2 创建、进程归组、采样和清理
├── workload.c   CPU、内存和 I/O 工作负载
├── sampler.c    PSI、cgroup 和 oslab_monitor 采样
├── report.c     summary.txt 和 Markdown 报告生成
├── util.c       文件系统、时间、字符串和错误辅助函数
└── main.c       命令分发
```

父进程负责参数解析、输出文件创建、cgroup 状态管理、启动工作负载子进程、周期采样、写入 CSV、生成摘要数据和执行本次运行范围内的清理。

工作负载子进程会等待父进程完成 cgroup 归组后再开始生成压力，从而保证 cgroup 指标对应当前运行。

## 3. 目录布局

```text
extension/tracebench/
├── Makefile
├── include/
│   └── tracebench.h
├── src/
│   ├── main.c
│   ├── args.c
│   ├── cgroup.c
│   ├── workload.c
│   ├── sampler.c
│   ├── report.c
│   └── util.c
└── tests/
    └── test_tracebench.sh
```

运行生成物写入用户指定输出目录，不提交到仓库。

## 4. 构建

`extension/tracebench/Makefile` 构建单个可执行文件：

```bash
cd extension/tracebench
make
```

编译设置：

```text
gcc
-Wall -Wextra -std=c11
-D_POSIX_C_SOURCE=200809L
-pthread
```

`make clean` 只删除构建产物，不删除运行数据。

## 5. 命令

```bash
./tracebench --help
sudo ./tracebench run --profile cpu --duration 5 --sample-interval 1 --output output/cpu
sudo ./tracebench run --profile memory --duration 5 --sample-interval 1 --output output/memory
sudo ./tracebench run --profile io --duration 5 --sample-interval 1 --output output/io
./tracebench report --input output/cpu --output output/cpu/report.md
sudo ./tracebench cleanup
```

默认 cgroup 模式需要 root。`--no-cgroup` 会禁用 cgroup 创建，并将 cgroup 字段写为 `NA`。

## 6. 核心数据模型

主配置：

```c
typedef enum {
    TB_CMD_HELP,
    TB_CMD_RUN,
    TB_CMD_REPORT,
    TB_CMD_CLEANUP
} TbCommand;

typedef enum {
    TB_PROFILE_CPU,
    TB_PROFILE_MEMORY,
    TB_PROFILE_IO
} TbProfile;

typedef struct {
    TbCommand command;
    TbProfile profile;
    int duration_sec;
    int sample_interval_sec;
    int cpu_workers;
    int memory_mb;
    int io_mb;
    int no_cgroup;
    int with_oslab_monitor;
    char cgroup_name[64];
    char output_path[512];
    char input_path[512];
    char report_output_path[512];
} TbConfig;
```

cgroup 状态：

```c
typedef struct {
    int enabled;
    char root_path[512];
    char profile_path[512];
    char run_path[512];
    char run_id[64];
} TbCgroup;
```

PSI 快照：

```c
typedef struct {
    int present;
    double avg10;
    double avg60;
    double avg300;
    unsigned long long total;
} TbPsiLine;
```

## 7. 运行流程

`tracebench run` 流程：

1. 解析并校验命令行参数。
2. 校验权限和必需内核接口。
3. 创建输出目录。
4. 写入 `command.txt`。
5. 写入 `environment.txt`。
6. 除非设置 `--no-cgroup`，否则创建 cgroup 路径。
7. fork 工作负载子进程。
8. 将工作负载子进程移入运行 cgroup。
9. 通知子进程开始生成压力。
10. 在 duration 结束前周期采样 PSI、cgroup 和可选 `oslab_monitor` 数据。
11. 停止并回收子进程。
12. 写入 `summary.txt`。
13. 删除空运行 cgroup 和临时 I/O 文件。

## 8. 工作负载

CPU：

- 子进程创建 `--cpu-workers` 个 pthread worker。
- worker 运行 CPU-bound 循环直到父进程控制的 duration 结束。

Memory：

- 子进程分配 `--memory-mb`。
- 重复触碰页面，使内存压力可见。

I/O：

- 子进程写入 `<output>/tracebench_io.tmp`。
- 写入后调用 `fsync`。
- 正常完成时删除临时文件。

## 9. 采样

PSI 来源：

```text
/proc/pressure/cpu
/proc/pressure/memory
/proc/pressure/io
```

cgroup 来源：

```text
cpu.stat
memory.current
memory.events
```

可选对照来源：

```text
/proc/oslab_monitor/overview
```

所有样本写入固定表头的 `samples.csv`。缺失的可选值写为 `NA`；`oslab_monitor` 缺失时使用 `oslab_monitor_available=false` 表示。

## 10. 报告生成

`tracebench report` 读取：

```text
<input>/samples.csv
```

它写入一个 Markdown 报告，包含：

- 运行配置。
- 环境引用。
- `samples.csv` 路径。
- PSI 摘要。
- cgroup 摘要。
- `oslab_monitor` 对照摘要。
- 局限性。

摘要统计使用简单的 `first`、`last`、`delta` 和 `max` 计算，不依赖绘图工具或外部报告生成器。

## 11. 清理边界

`tracebench cleanup` 的范围刻意保持狭窄。它可以删除：

```text
/sys/fs/cgroup/<cgroup-name>/
extension/tracebench/output/*/tracebench_io.tmp
```

它不得删除：

- `samples.csv`
- `summary.txt`
- Markdown 报告
- 任意用户文件
- 用户请求的输出目录

如果 cgroup 中仍有进程，cleanup 会跳过或失败，而不是杀死无关进程。

## 12. 错误处理

所有失败路径输出 `error:` 并返回非零退出码。

示例：

```text
error: --profile must be one of cpu, memory, io
error: cgroup mode requires root; rerun with sudo or use --no-cgroup to collect without cgroup metrics
error: /proc/pressure/cpu not found
error: samples.csv not found under output/cpu
```

## 13. 验证

集成脚本：

```bash
cd extension/tracebench
sudo bash tests/test_tracebench.sh
```

它验证：

- 构建。
- help 输出。
- 非法参数处理。
- CPU、内存和 I/O 运行。
- cgroup 启用模式和 no-cgroup 模式。
- PSI、cgroup 和 `oslab_monitor` CSV 字段。
- CSV 字段数量一致性。
- summary 和 Markdown 报告生成。
- cleanup 行为。

## 14. 设计决策

### DD-001：保持 TraceBench 独立

TraceBench 位于 `extension/tracebench/`，不修改 `basic/` 或 `extension/oslab_monitor/`。它可以读取 `oslab_monitor` 输出，但不要求模块必须加载。

### DD-002：cgroup 模式显式要求 root

默认 cgroup 模式要求 root。工具不会在内部调用 `sudo`，避免隐藏提权和意外的部分数据采集。

### DD-003：父进程采样，子进程施压

父进程负责采样和清理，子进程负责生成压力。同步管道确保压力只在 cgroup 归组后开始。

### DD-004：保持文本输出

工具输出文本、CSV 和 Markdown，从而保持依赖少、终端可检查。

### DD-005：限制清理范围

清理只覆盖 TraceBench 拥有的 cgroup 和精确命名的临时文件。生成的数据材料默认保留。
