# PRD v2：OSLab TraceBench 资源压力与系统观测实验平台

## 1. 项目背景

当前 OSLab TraceBench 已经完成课程基础 PRD：基础四个 C 命令行模块能够模拟调度、内存、同步和文件系统机制，扩展部分也已经通过 Linux 内核模块 `/proc/oslab_monitor/` 观察真实系统的进程和内存状态。

v2 阶段不再继续简单堆叠基础算法，而是把方向深化为“真实 Linux 资源压力实验与系统观测”。目标是让学生不仅能在用户态程序里看到算法结果，还能在 Ubuntu VM 中制造 CPU、内存、I/O 压力，使用 cgroup v2、PSI、`/proc`、tracefs 或 bpftrace 观察真实 Linux 系统如何表现，并自动生成可复现实验数据和报告材料。

本阶段项目名称为：

```text
OSLab TraceBench
```

含义：

- `OSLab`：延续操作系统课程设计定位。
- `TraceBench`：围绕 tracing、benchmark、可复现实验数据组织功能。

## 2. 外部调研依据

### 2.1 Linux PSI

Linux PSI（Pressure Stall Information）用于衡量 CPU、内存和 I/O 资源不足导致任务等待的时间比例。Linux 官方文档说明 PSI 可以通过 `/proc/pressure/` 暴露系统级压力数据，也可以通过 cgroup v2 暴露分组级压力数据。

参考：

- Linux PSI 官方文档：https://docs.kernel.org/accounting/psi.html
- Kubernetes PSI 指标说明：https://kubernetes.io/docs/reference/instrumentation/understand-psi-metrics/

对本项目的启发：

- CPU 调度、内存管理和 I/O 不应只停留在模拟结果，还可以用真实系统压力指标观察。
- PSI 指标输出文本稳定，适合课程实验采样、CSV 留存和报告分析。
- PSI 不要求修改内核，适合 Ubuntu VM 课程环境。

### 2.2 cgroup v2

cgroup v2 是 Linux 统一资源控制接口，支持 CPU、内存、I/O 等资源限制和统计。官方文档中包含 `cpu.max`、`cpu.stat`、`memory.current`、`memory.high`、`memory.max`、`memory.events` 等接口。

参考：

- Linux cgroup v2 官方文档：https://docs.kernel.org/admin-guide/cgroup-v2.html

对本项目的启发：

- 可以把 workload 放入独立 cgroup，避免实验压力影响整台 VM。
- 可以通过 CPU 限额和内存上限制造可控压力。
- 可以把课程中的资源管理概念映射到真实 Linux 控制接口。

### 2.3 ftrace / tracefs

ftrace 和 tracefs 是 Linux 官方跟踪设施，支持通过 trace events 观察调度、系统调用、块 I/O 等内核事件。

参考：

- ftrace 官方文档：https://docs.kernel.org/trace/ftrace.html
- trace events 官方文档：https://docs.kernel.org/trace/events.html

对本项目的启发：

- 可以用 `sched_switch`、`sched_wakeup` 等事件解释上下文切换和调度行为。
- tracefs 可以作为 P1 增强项，不必放入 v2 最小必做范围。
- 由于 tracefs 权限、字段和内核配置差异较多，必须作为可降级能力设计。

### 2.4 bpftrace / eBPF

bpftrace 是基于 eBPF 的高级跟踪工具，适合快速编写内核和用户态 tracing 脚本。bpftrace 官方仓库和文档都面向现代 Linux 观测实践。

参考：

- bpftrace 文档：https://bpftrace.org/docs/
- bpftrace 仓库：https://github.com/bpftrace/bpftrace

对本项目的启发：

- bpftrace 可以为调度延迟、off-CPU 时间、系统调用统计提供现代观测能力。
- 但 bpftrace 依赖内核配置和包安装，不适合作为 v2 P0 必做。
- 可作为 P1 增强项，允许没有 bpftrace 的环境自动跳过。

### 2.5 sched_ext

sched_ext 允许用 BPF 编写 Linux 调度器，创新性很强，但 Linux 官方文档也说明其调度器 ABI 仍可能变化。

参考：

- sched_ext 官方文档：https://docs.kernel.org/scheduler/sched-ext.html
- sched_ext 示例仓库：https://github.com/sched-ext/scx

对本项目的启发：

- sched_ext 与本项目调度模块主题高度相关。
- 当前项目已验证环境为 Ubuntu 24.04.2、Linux 6.11.0-17-generic，不一定具备 sched_ext 主线环境。
- 因此 sched_ext 只能作为 P2 挑战项，不能作为 v2 主要验收条件。

## 3. v2 定位

v2 定位为：

```text
面向操作系统课程设计的 Linux 资源压力、运行态观测和自动报告平台。
```

v2 不是：

- 不是替换当前基础四模块。
- 不是重新实现 Prometheus、Grafana 或生产级监控系统。
- 不是修改 Linux 内核源码。
- 不是真实 Linux 调度器改造。
- 不是 Web 平台或 GUI 平台。

v2 应该做到：

- 能自动制造可控压力。
- 能采集真实 Linux 指标。
- 能和现有 `/proc/oslab_monitor/` 输出形成对照。
- 能把实验结果保存为 CSV 和 Markdown 报告。
- 能在 Ubuntu VM 中一键演示和验收。

## 4. 用户与使用场景

### 4.1 目标用户

- 学生：运行实验、观察资源压力、整理报告。
- 助教：快速复现实验、检查输出文件和报告结论。
- 教师：评估项目是否从基础模拟扩展到真实系统实践。

### 4.2 核心使用场景

- 学生运行 CPU 压力实验，观察 CPU PSI、进程状态、调度事件和 cgroup CPU 统计变化。
- 学生运行内存压力实验，观察 `memory.current`、`memory.events`、内存 PSI 和 `/proc/oslab_monitor/overview` 的变化。
- 学生运行 I/O 压力实验，观察 I/O PSI 和文件写入压力对系统的影响。
- 学生通过统一命令生成 CSV 数据和 Markdown 报告，把结果直接放入课程报告。
- 助教通过一条测试命令确认 v2 功能是否可运行，并检查输出字段是否齐全。

## 5. 范围划分

### 5.1 P0 必做范围

P0 是 v2 的最小可验收版本，必须实现。

- 新增用户态工具 `tracebench`。
- 支持 CPU、memory、io 三类 workload。
- 支持创建、使用和清理独立 cgroup v2 实验目录。
- 支持采集系统级 PSI。
- 支持采集实验 cgroup 的 CPU 和内存指标。
- 支持采集现有 `/proc/oslab_monitor/overview` 作为对照。
- 支持输出 CSV 采样文件。
- 支持输出 Markdown 实验报告。
- 支持 Bash 集成测试。

### 5.2 P1 增强范围

P1 作为增强项实现，允许环境不支持时自动跳过并在报告中说明。

- tracefs 调度事件采样。
- bpftrace 脚本采样。
- 对采样 CSV 做简单汇总统计。
- 增加更多 workload 参数，例如线程数、运行时长、内存增长步长、I/O 文件大小。

### 5.3 P2 挑战范围

P2 作为挑战项，不纳入默认验收。

- sched_ext BPF 调度器实验。
- 对比默认 Linux 调度器与自定义简化调度策略。
- 生成调度策略对比报告。

## 6. 推荐目录结构

v2 新增内容放在 `extension/tracebench/`，避免影响已完成的基础 PRD。

```text
extension/
└── tracebench/
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
    ├── scripts/
    │   ├── run_cpu_demo.sh
    │   ├── run_memory_demo.sh
    │   ├── run_io_demo.sh
    │   └── cleanup.sh
    ├── bpftrace/
    │   ├── sched_latency.bt
    │   └── syscall_count.bt
    ├── tests/
    │   └── test_tracebench.sh
    └── output/
        └── .gitkeep
```

目录约束：

- `extension/tracebench/src/` 只放用户态 C 代码。
- `extension/tracebench/bpftrace/` 只放可选 bpftrace 脚本。
- `extension/tracebench/output/` 用于本地实验输出，默认不提交生成的 CSV 和报告。
- 不修改 `basic/` 四模块作为 v2 的依赖。
- 不要求 `oslab_monitor.ko` 必须加载，但如果已加载，`tracebench` 必须采集其 `overview` 作为对照。

## 7. 技术栈与运行环境

### 7.1 P0 环境

P0 默认环境：

```text
Ubuntu 24.04 LTS VM
Linux 6.11 或同级 Ubuntu 默认内核
gcc
make
bash
cgroup v2
/proc/pressure/*
```

### 7.2 P1 环境

P1 可选环境：

```text
tracefs mounted at /sys/kernel/tracing or /sys/kernel/debug/tracing
bpftrace
kernel headers or BTF support
root permission for tracing
```

### 7.3 权限约束

- 读取 PSI 通常不需要 root。
- 创建和配置 cgroup v2 可能需要 root，测试脚本可使用 `sudo`。
- tracefs 和 bpftrace 通常需要 root。
- `tracebench` 必须在权限不足时输出 `error:` 并说明缺少的权限或接口路径。

## 8. 命令行需求

### 8.1 主命令

P0 必须生成可执行文件：

```bash
extension/tracebench/tracebench
```

命令格式：

```bash
./tracebench --help
./tracebench run --profile cpu --duration 5 --sample-interval 1 --output output/cpu
./tracebench run --profile memory --duration 5 --sample-interval 1 --output output/memory
./tracebench run --profile io --duration 5 --sample-interval 1 --output output/io
./tracebench report --input output/cpu --output output/cpu_report.md
./tracebench cleanup
```

### 8.2 参数规则

- `--profile` 必须为 `cpu`、`memory`、`io`。
- `--duration` 单位为秒，必须大于 `0`。
- `--sample-interval` 单位为秒，必须大于 `0`，且不得大于 `duration`。
- `--output` 指定输出目录，目录不存在时自动创建。
- 所有错误输出必须以 `error:` 开头。
- 正常完成返回 `0`，错误返回非 `0`。

### 8.3 profile 参数

P0 必须支持以下 profile 参数，并提供默认值：

```bash
--cpu-workers N
--memory-mb N
--io-mb N
--cgroup-name NAME
--no-cgroup
--with-oslab-monitor
```

默认值：

| 参数 | 默认值 | 说明 |
|---|---:|---|
| `--cpu-workers` | `2` | CPU 压力线程数 |
| `--memory-mb` | `128` | 内存压力申请量 |
| `--io-mb` | `64` | I/O 临时文件写入量 |
| `--cgroup-name` | `oslab_tracebench` | cgroup v2 根实验组名称 |

参数规则：

- `--cpu-workers N`：CPU 压力线程数，`N > 0`。
- `--memory-mb N`：内存压力大小，`N > 0`，默认测试不得超过 `512`。
- `--io-mb N`：I/O 写入大小，`N > 0`。
- `--cgroup-name NAME`：只允许字母、数字、下划线和短横线。
- `--no-cgroup`：跳过 cgroup 创建，报告中必须说明未采集 cgroup 指标；该模式只用于低权限演示，不作为 P0 验收路径。
- `--with-oslab-monitor`：要求 `/proc/oslab_monitor/overview` 必须存在，否则报错。

示例：

```bash
sudo ./tracebench run --profile cpu --duration 5 --sample-interval 1 --cpu-workers 2 --output output/cpu
sudo ./tracebench run --profile memory --duration 5 --sample-interval 1 --memory-mb 128 --output output/memory
sudo ./tracebench run --profile io --duration 5 --sample-interval 1 --io-mb 64 --output output/io
```

## 9. P0 功能需求

### 9.1 CLI 与运行控制

- V2-CLI-FR-1：系统必须提供 `tracebench --help`。
- V2-CLI-FR-2：系统必须提供 `tracebench run` 子命令。
- V2-CLI-FR-3：系统必须提供 `tracebench report` 子命令。
- V2-CLI-FR-4：系统必须提供 `tracebench cleanup` 子命令。
- V2-CLI-FR-5：参数错误时必须输出 `error:` 并返回非 `0`。
- V2-CLI-FR-6：实验运行结束后必须回收自身启动的 workload 进程。

验收标准：

- V2-CLI-AC-1：`./tracebench --help` 输出包含 `run`、`report`、`cleanup`。
- V2-CLI-AC-2：非法 profile 返回非 `0`，输出包含 `error:`。
- V2-CLI-AC-3：实验结束后不存在由本次 `tracebench` 遗留的压力进程。

### 9.2 Workload 生成

- V2-WL-FR-1：CPU profile 必须启动 CPU 密集型 workload。
- V2-WL-FR-2：memory profile 必须启动内存分配和访问 workload。
- V2-WL-FR-3：io profile 必须启动文件写入或读写 workload。
- V2-WL-FR-4：每类 workload 必须支持按 `duration` 自动结束。
- V2-WL-FR-5：workload 必须由项目自身实现，不能强依赖 `stress-ng`、`fio` 等外部压测工具。
- V2-WL-FR-6：I/O workload 的临时文件必须写入输出目录或系统临时目录，并在 cleanup 中清理。

验收标准：

- V2-WL-AC-1：CPU profile 运行时采样文件中 CPU PSI 或 CPU cgroup 指标发生变化。
- V2-WL-AC-2：memory profile 运行时采样文件中 memory 当前值或 memory PSI 发生变化。
- V2-WL-AC-3：io profile 运行时采样文件中 I/O PSI 或临时文件大小发生变化。
- V2-WL-AC-4：运行 `duration=3` 的实验应在 6 秒内结束。

### 9.3 cgroup v2 管理

- V2-CG-FR-1：系统必须检测当前系统是否启用 cgroup v2。
- V2-CG-FR-2：系统必须为每次实验创建独立 cgroup 目录。
- V2-CG-FR-3：系统必须把 workload 进程加入该 cgroup。
- V2-CG-FR-4：系统必须在实验结束后移除空 cgroup。
- V2-CG-FR-5：如果 cgroup 创建失败，必须输出明确错误原因。
- V2-CG-FR-6：系统必须采集 `cpu.stat`。
- V2-CG-FR-7：系统必须采集 `memory.current`。
- V2-CG-FR-8：系统必须采集 `memory.events`。

验收标准：

- V2-CG-AC-1：实验期间 cgroup 目录存在。
- V2-CG-AC-2：实验结束后默认清理 cgroup 目录。
- V2-CG-AC-3：CSV 中包含 `cgroup_cpu_usage_usec` 或等价 CPU 统计字段。
- V2-CG-AC-4：CSV 中包含 `cgroup_memory_current`。
- V2-CG-AC-5：权限不足时输出 `error:`，不静默降级。

### 9.4 PSI 采样

- V2-PSI-FR-1：系统必须读取 `/proc/pressure/cpu`。
- V2-PSI-FR-2：系统必须读取 `/proc/pressure/memory`。
- V2-PSI-FR-3：系统必须读取 `/proc/pressure/io`。
- V2-PSI-FR-4：系统必须解析 `some` 行中的 `avg10`、`avg60`、`avg300`、`total`。
- V2-PSI-FR-5：系统必须解析 `full` 行中的 `avg10`、`avg60`、`avg300`、`total`；若某资源没有 `full` 行，必须输出空值或 `NA`。
- V2-PSI-FR-6：系统必须按 `sample-interval` 周期采样。

验收标准：

- V2-PSI-AC-1：CSV 中包含 CPU、memory、I/O 的 `some_avg10` 字段。
- V2-PSI-AC-2：CSV 中包含 CPU、memory、I/O 的 `some_total` 字段。
- V2-PSI-AC-3：`duration=5`、`sample-interval=1` 时至少产生 5 行采样。
- V2-PSI-AC-4：如果 `/proc/pressure/*` 不存在，命令返回非 `0` 并提示内核不支持 PSI。

### 9.5 `/proc/oslab_monitor` 对照采样

- V2-OM-FR-1：如果 `/proc/oslab_monitor/overview` 存在，系统必须采集其关键字段。
- V2-OM-FR-2：关键字段至少包括 `total_tasks`、`running_tasks`、`sleeping_tasks`、`mem_free_kb`、`mem_available_kb`。
- V2-OM-FR-3：如果 `/proc/oslab_monitor/overview` 不存在，默认不报错，但 CSV 中必须记录 `oslab_monitor_available=false`。
- V2-OM-FR-4：如果用户指定 `--with-oslab-monitor`，但接口不存在，必须报错。

验收标准：

- V2-OM-AC-1：加载 `oslab_monitor.ko` 后运行实验，CSV 中包含 `oslab_total_tasks`。
- V2-OM-AC-2：未加载模块时运行实验，CSV 中包含 `oslab_monitor_available=false`。
- V2-OM-AC-3：`--with-oslab-monitor` 在模块未加载时返回非 `0`。

### 9.6 CSV 输出

- V2-CSV-FR-1：每次 run 必须生成 `samples.csv`。
- V2-CSV-FR-2：CSV 第一行必须为稳定表头。
- V2-CSV-FR-3：每行必须包含采样时间戳或相对秒数。
- V2-CSV-FR-4：每行必须包含 profile、duration、sample index。
- V2-CSV-FR-5：CSV 字段顺序必须稳定，便于脚本测试和报告引用。

最低字段要求：

```text
sample_index
elapsed_ms
profile
cpu_some_avg10
cpu_some_total
memory_some_avg10
memory_some_total
io_some_avg10
io_some_total
cgroup_cpu_usage_usec
cgroup_memory_current
cgroup_memory_events_low
cgroup_memory_events_high
cgroup_memory_events_max
cgroup_memory_events_oom
oslab_monitor_available
oslab_total_tasks
oslab_running_tasks
oslab_sleeping_tasks
oslab_mem_free_kb
oslab_mem_available_kb
```

验收标准：

- V2-CSV-AC-1：`samples.csv` 存在。
- V2-CSV-AC-2：表头包含最低字段要求。
- V2-CSV-AC-3：采样行数不少于预期最小行数。
- V2-CSV-AC-4：字段数量每行一致。

### 9.7 Markdown 报告输出

- V2-RPT-FR-1：`tracebench report` 必须读取指定输出目录中的 `samples.csv`。
- V2-RPT-FR-2：系统必须生成 Markdown 报告。
- V2-RPT-FR-3：报告必须包含实验环境。
- V2-RPT-FR-4：报告必须包含 profile、duration、sample interval。
- V2-RPT-FR-5：报告必须包含 PSI 指标摘要。
- V2-RPT-FR-6：报告必须包含 cgroup 指标摘要。
- V2-RPT-FR-7：报告必须包含 `/proc/oslab_monitor` 是否可用。
- V2-RPT-FR-8：报告必须包含适合课程报告引用的结果分析段落。

报告建议结构：

```markdown
# OSLab TraceBench 实验报告

## 1. 实验配置
## 2. 运行环境
## 3. 采样文件
## 4. PSI 资源压力摘要
## 5. cgroup 指标摘要
## 6. oslab_monitor 对照结果
## 7. 实验现象分析
## 8. 局限性
```

验收标准：

- V2-RPT-AC-1：`tracebench report` 成功生成 `.md` 文件。
- V2-RPT-AC-2：报告包含 `PSI`、`cgroup`、`oslab_monitor` 三类关键词。
- V2-RPT-AC-3：报告中必须引用 `samples.csv` 路径。

## 10. P1 增强需求

### 10.1 tracefs 调度事件采样

- V2-TR-FR-1：系统可以检测 tracefs 是否可用。
- V2-TR-FR-2：系统可以启用 `sched_switch` 事件。
- V2-TR-FR-3：系统可以在实验期间采样 trace 输出。
- V2-TR-FR-4：系统必须在实验结束后恢复 tracefs 原状态或明确关闭本次启用的事件。
- V2-TR-FR-5：tracefs 不可用时必须跳过并在报告中说明。

验收标准：

- V2-TR-AC-1：tracefs 可用时输出 `trace_sched.csv` 或等价文件。
- V2-TR-AC-2：tracefs 不可用时主实验仍可完成。

### 10.2 bpftrace 脚本

- V2-BPF-FR-1：项目可以提供 `sched_latency.bt`。
- V2-BPF-FR-2：项目可以提供 `syscall_count.bt`。
- V2-BPF-FR-3：`tracebench` 可以检测 bpftrace 是否安装。
- V2-BPF-FR-4：bpftrace 不存在时必须跳过并在报告中说明。

验收标准：

- V2-BPF-AC-1：安装 bpftrace 的环境中脚本可运行并输出结果。
- V2-BPF-AC-2：未安装 bpftrace 时 P0 测试不失败。

## 11. P2 挑战需求

### 11.1 sched_ext 调度器实验

- V2-SCX-FR-1：项目可以新增 `extension/sched_ext_lab/`。
- V2-SCX-FR-2：项目可以基于 sched_ext 示例实现一个极简调度策略。
- V2-SCX-FR-3：系统可以对比默认调度器和自定义调度器下的 TraceBench 结果。
- V2-SCX-FR-4：该功能必须明确标注为挑战项。

验收标准：

- V2-SCX-AC-1：仅在支持 sched_ext 的内核中运行。
- V2-SCX-AC-2：不支持 sched_ext 的环境不影响 P0/P1。
- V2-SCX-AC-3：报告必须说明内核版本和 sched_ext ABI 风险。

## 12. 非功能需求

### 12.1 可复现性

- V2-NFR-1：相同参数必须生成相同结构的输出目录。
- V2-NFR-2：每次实验必须记录命令参数。
- V2-NFR-3：每次实验必须记录系统环境，包括 `uname -a`、发行版、gcc 版本。

### 12.2 安全性

- V2-NFR-4：不得杀死非本次实验启动的进程。
- V2-NFR-5：不得删除用户未指定的目录。
- V2-NFR-6：cleanup 只能清理项目命名空间内的 cgroup、临时文件和输出文件。
- V2-NFR-7：所有路径操作必须限制在用户指定输出目录或项目临时目录内。

### 12.3 可靠性

- V2-NFR-8：实验中断时必须尽量清理 workload。
- V2-NFR-9：Bash 测试必须使用 `trap` 执行清理。
- V2-NFR-10：采样过程中单个可选指标失败不能破坏 P0 必需指标输出。

### 12.4 简洁性

- V2-NFR-11：不引入 Python、Node.js 或数据库作为 P0 必需依赖。
- V2-NFR-12：P0 只使用 C、Makefile、Bash 和 Linux 系统接口。
- V2-NFR-13：输出格式保持文本、CSV 和 Markdown，不引入 GUI。

## 13. 测试要求

### 13.1 单元级检查

P0 至少覆盖：

- 参数解析错误。
- PSI 文本解析。
- cgroup 指标文本解析。
- CSV 表头和行字段数量。
- 报告生成。

### 13.2 集成测试

新增脚本：

```bash
extension/tracebench/tests/test_tracebench.sh
```

测试内容：

- `make` 成功生成 `tracebench`。
- `./tracebench --help` 输出关键命令。
- CPU profile 运行成功。
- memory profile 运行成功。
- io profile 运行成功。
- 每个 profile 生成 `samples.csv`。
- `tracebench report` 生成 Markdown 报告。
- 错误参数输出 `error:`。
- `cleanup` 不报错。

### 13.3 推荐测试命令

```bash
cd extension/tracebench
make
./tracebench --help
sudo ./tracebench run --profile cpu --duration 3 --sample-interval 1 --output output/cpu
./tracebench report --input output/cpu --output output/cpu_report.md
sudo ./tracebench cleanup
bash tests/test_tracebench.sh
make clean
```

### 13.4 验收环境

P0 验收环境：

```text
Ubuntu 24.04 LTS VM
cgroup v2 enabled
/proc/pressure/cpu present
/proc/pressure/memory present
/proc/pressure/io present
```

如果课程环境只提供 Ubuntu 22.04，需要在报告中记录以下检查结果：

```bash
mount | grep cgroup2
ls /proc/pressure
uname -a
```

## 14. 输出文件规范

一次 CPU 实验输出目录示例：

```text
extension/tracebench/output/cpu/
├── command.txt
├── environment.txt
├── samples.csv
├── summary.txt
└── report.md
```

文件要求：

- `command.txt`：记录完整运行命令。
- `environment.txt`：记录系统环境。
- `samples.csv`：记录采样数据。
- `summary.txt`：记录关键指标摘要。
- `report.md`：记录 Markdown 实验报告。

生成文件默认不提交到 Git；只提交 `.gitkeep` 或 README 说明。

## 15. 与现有项目的关系

### 15.1 保持不变

- `basic/scheduler/`
- `basic/memory/`
- `basic/sync/`
- `basic/filesystem/`
- `extension/oslab_monitor/`
- `tests/run_all.sh`
- `docs/PRD.md`

### 15.2 可复用内容

- 可复用 `extension/oslab_monitor/` 的 `/proc/oslab_monitor/overview` 输出作为对照指标。
- 可复用当前 Ubuntu VM 验证流程。
- 可在课程报告 v2 中引用当前 `docs/COURSE_REPORT.md` 的基础部分结果。

### 15.3 新增文档建议

后续实现前建议新增：

- `docs/TECHv2.md`
- `docs/PLANv2.md`
- `docs/TRACEBENCH_REPORT_TEMPLATE.md`

## 16. 非目标

v2 不包含：

- 不修改 Linux 内核源码。
- 不重新编译或替换 Linux 内核。
- 不要求使用 sched_ext 作为默认验收。
- 不实现 Web UI。
- 不引入 Prometheus、Grafana、ElasticSearch 等外部监控平台。
- 不强依赖 `stress-ng`、`fio`、`perf`、`bpftrace`。
- 不让基础四模块依赖 TraceBench。
- 不将 WSL2 作为默认验收环境。
- 不采集用户隐私内容、命令行参数中的敏感文本或文件内容。

## 17. 用户故事

### US-V2-001：运行 CPU 压力实验

描述：作为学生，我希望一条命令启动 CPU 压力实验并采样 PSI 和 cgroup 指标，以便观察 CPU 资源竞争对系统的影响。

验收标准：

- 能运行 `tracebench run --profile cpu`。
- 能生成 `samples.csv`。
- CSV 中包含 CPU PSI 和 cgroup CPU 指标。
- 实验结束后 workload 被清理。

### US-V2-002：运行内存压力实验

描述：作为学生，我希望启动内存压力实验，以便观察内存占用、内存事件和内存 PSI 的变化。

验收标准：

- 能运行 `tracebench run --profile memory`。
- CSV 中包含 `cgroup_memory_current`。
- CSV 中包含 memory PSI 字段。
- Markdown 报告包含内存压力分析段落。

### US-V2-003：运行 I/O 压力实验

描述：作为学生，我希望启动 I/O 压力实验，以便观察 I/O 等待和 PSI 指标变化。

验收标准：

- 能运行 `tracebench run --profile io`。
- 生成临时 I/O 文件并在 cleanup 中清理。
- CSV 中包含 I/O PSI 字段。
- 报告说明 I/O workload 的文件大小和路径。

### US-V2-004：生成课程报告材料

描述：作为学生，我希望从采样数据自动生成 Markdown 报告，以便快速整理课程设计扩展部分成果。

验收标准：

- `tracebench report` 能读取 `samples.csv`。
- 报告包含实验配置、环境、指标摘要和分析。
- 报告可以直接复制到课程设计报告中。

### US-V2-005：对照 oslab_monitor

描述：作为学生，我希望 TraceBench 能读取现有 `/proc/oslab_monitor/overview`，以便把自研内核模块输出和 Linux 标准接口采样结果放在同一份数据中。

验收标准：

- 模块加载时 CSV 包含 oslab 指标。
- 模块未加载时默认不失败。
- 指定 `--with-oslab-monitor` 时模块未加载必须失败。

### US-V2-006：可选 tracing 增强

描述：作为进阶学生，我希望在支持 tracefs 或 bpftrace 的环境中采样调度事件，以便进一步分析上下文切换和调度延迟。

验收标准：

- tracefs 或 bpftrace 可用时生成额外 trace 文件。
- 不可用时 P0 实验仍可完成。
- 报告明确说明增强采样是否启用。

## 18. 里程碑

### M1：需求与技术方案完成

完成条件：

- `docs/PRDv2.md` 完成。
- `docs/TECHv2.md` 完成。
- `docs/PLANv2.md` 完成。

### M2：TraceBench P0 CLI 完成

完成条件：

- `tracebench --help` 可用。
- CPU、memory、io profile 可运行。
- 参数错误处理完成。

### M3：采样与输出完成

完成条件：

- PSI 采样完成。
- cgroup 指标采样完成。
- oslab_monitor 对照采样完成。
- `samples.csv` 输出稳定。

### M4：报告与测试完成

完成条件：

- Markdown 报告生成完成。
- `tests/test_tracebench.sh` 通过。
- README 或 v2 文档补充运行方式。

### M5：P1 可选增强

完成条件：

- tracefs 或 bpftrace 至少实现一项。
- 不支持环境可跳过。
- 报告中能说明增强采样结果或跳过原因。

## 19. 最终验收清单

P0 必须满足：

- [ ] `docs/PRDv2.md` 存在。
- [ ] `extension/tracebench/Makefile` 存在。
- [ ] `make` 能生成 `tracebench`。
- [ ] `tracebench --help` 可运行。
- [ ] CPU profile 可运行。
- [ ] memory profile 可运行。
- [ ] io profile 可运行。
- [ ] 每个 profile 生成 `samples.csv`。
- [ ] CSV 包含 PSI 字段。
- [ ] CSV 包含 cgroup 字段。
- [ ] CSV 包含 oslab_monitor 可用性字段。
- [ ] `tracebench report` 能生成 Markdown 报告。
- [ ] 错误参数输出 `error:`。
- [ ] `tracebench cleanup` 能清理实验产物。
- [ ] `bash tests/test_tracebench.sh` 通过。
- [ ] 文档说明 P1/P2 的环境限制。

## 20. 默认决策

为保证 v2 需求明确，以下决策作为 TECHv2 和 PLANv2 的默认输入。

| 编号 | 决策 | 说明 |
|---|---|---|
| V2-DEC-1 | P0 默认使用 `sudo` 运行 `run` 和 `cleanup` | 降低 cgroup v2 权限差异带来的实现复杂度 |
| V2-DEC-2 | cgroup v2 根路径固定为 `/sys/fs/cgroup/oslab_tracebench/` | 每次实验在其下创建 profile 和 run id 子目录 |
| V2-DEC-3 | CPU workload 默认 2 个 worker | 保证双核和多核 VM 都能稳定运行 |
| V2-DEC-4 | memory workload 默认 128 MB | 避免课程 VM 默认配置下触发不可控 OOM |
| V2-DEC-5 | memory workload 默认测试上限为 512 MB | 用户显式提高时必须在报告中记录参数 |
| V2-DEC-6 | I/O workload 默认写入 64 MB 临时文件 | 能产生观测变化，又不会明显拖慢验收 |
| V2-DEC-7 | I/O workload 写入后执行 `fsync` | 让 I/O 压力更容易在 PSI 中体现 |
| V2-DEC-8 | Markdown 报告只生成文本摘要和表格 | 不生成图片，不引入图表依赖 |
| V2-DEC-9 | P1 优先实现 tracefs，再实现 bpftrace | tracefs 是内核官方接口，额外依赖少于 bpftrace |
| V2-DEC-10 | sched_ext 只作为 P2 挑战项 | 当前 Ubuntu 6.11 验证环境不保证支持 sched_ext |
