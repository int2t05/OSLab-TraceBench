# OSLab TraceBench v2 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 按 `docs/PRDv2.md` 和 `docs/TECHv2.md` 实现 TraceBench v2 P0：新增 `extension/tracebench/tracebench`，支持 CPU、memory、io 资源压力实验，采集 PSI、cgroup v2 和 oslab_monitor 对照指标，输出 CSV、summary 和 Markdown 报告，并提供 Bash 集成测试。

**Architecture:** v2 新增内容全部位于 `extension/tracebench/`，不修改 `basic/` 四模块，也不让基础模块依赖 TraceBench。`tracebench` 采用用户态 C CLI：父进程负责参数解析、cgroup 管理、周期采样、CSV/报告输出和 cleanup；workload 子进程负责 CPU、memory、io 压力生成，并通过 pipe 保证加入 cgroup 后才开始施压。P1 tracefs、bpftrace 和 P2 sched_ext 只作为可降级增强或挑战项预留，不纳入 P0 默认验收。

**Tech Stack:** C、Makefile、POSIX pthread、fork/waitpid、pipe、signal、clock_gettime、Linux cgroup v2、Linux PSI `/proc/pressure/*`、`/proc/oslab_monitor/overview`、Bash、Ubuntu 24.04 LTS VM。

---

## 1. 计划边界

本文档只说明要编写或修改的代码文件、测试文件、脚本文件、文档文件、测试命令、预期结果和完成条件，不包含实际 C 源码实现。

实现必须遵守：

- 以 `docs/PRDv2.md` 和 `docs/TECHv2.md` 为准。
- P0 是默认实现和默认验收范围。
- P1 tracefs、bpftrace 和 P2 sched_ext 不进入 P0 默认任务，只在预留章节说明。
- 不修改 `basic/scheduler/`、`basic/memory/`、`basic/sync/`、`basic/filesystem/`。
- 不修改 `extension/oslab_monitor/` 的现有行为；只读取 `/proc/oslab_monitor/overview` 作为可选对照。
- 不引入 Python、Node.js、数据库、Prometheus、Grafana、`stress-ng`、`fio`、`perf` 或 `bpftrace` 作为 P0 必需依赖。
- P0 默认 `run` 和 `cleanup` 使用 `sudo`，权限不足时必须输出 `error:` 并返回非 `0`。
- `--no-cgroup` 只作为低权限演示，不作为 P0 完整验收路径。
- 所有 CLI 错误输出以 `error:` 开头。
- 所有 C、H、SH 文件必须有中文文件头注释，说明文件为什么存在、解决什么问题、为什么放在当前目录。
- Bash 脚本必须使用中文注释解释关键 why，尤其是 `trap cleanup EXIT`。
- `.gitignore` 如需新增规则，只能从文件末尾追加，不能覆盖原文件。

## 2. P0 验收环境

P0 默认验收环境：

```text
Ubuntu 24.04 LTS VM
Linux 6.11 或同级 Ubuntu 默认内核
gcc
make
bash
cgroup v2
/proc/pressure/cpu
/proc/pressure/memory
/proc/pressure/io
```

实现前在 Ubuntu VM 中检查：

```bash
uname -a
gcc --version | head -1
make --version | head -1
mount | grep cgroup2
test -r /proc/pressure/cpu
test -r /proc/pressure/memory
test -r /proc/pressure/io
```

Expected:

- `mount | grep cgroup2` 能看到 cgroup2 挂载。
- 三个 `/proc/pressure/*` 文件均可读。
- 若这些条件不满足，不能宣称 P0 验收通过。

## 3. 实现顺序依赖

1. 创建 `extension/tracebench/` 骨架、输出目录、P1 预留目录和 `.gitignore` 追加规则。
2. 实现公共头文件、Makefile、基础 CLI、参数解析、`--help` 和参数错误处理。
3. 实现输出目录创建、`command.txt` 和 `environment.txt`。
4. 实现 PSI 文本解析和三类 PSI 采样。
5. 实现 cgroup v2 检测、创建、加入进程、指标读取和单次 run 清理。
6. 实现 `run` 控制器、父子进程同步和 CPU workload。
7. 实现 memory workload。
8. 实现 I/O workload 和临时文件清理。
9. 实现周期采样和稳定表头 `samples.csv`。
10. 实现 `/proc/oslab_monitor/overview` 对照采样。
11. 实现 `summary.txt` 和 `tracebench report` Markdown 报告。
12. 实现 `tracebench cleanup`。
13. 编写 demo scripts。
14. 编写并运行 P0 集成测试。
15. 同步 README、FEATURES、DOC_AUDIT，并新增报告模板。

## 4. 文件总览

### 4.1 新增 TraceBench 模块

- Create: `extension/tracebench/Makefile`
- Create: `extension/tracebench/include/tracebench.h`
- Create: `extension/tracebench/src/main.c`
- Create: `extension/tracebench/src/args.c`
- Create: `extension/tracebench/src/cgroup.c`
- Create: `extension/tracebench/src/workload.c`
- Create: `extension/tracebench/src/sampler.c`
- Create: `extension/tracebench/src/report.c`
- Create: `extension/tracebench/src/util.c`
- Create: `extension/tracebench/scripts/run_cpu_demo.sh`
- Create: `extension/tracebench/scripts/run_memory_demo.sh`
- Create: `extension/tracebench/scripts/run_io_demo.sh`
- Create: `extension/tracebench/scripts/cleanup.sh`
- Create: `extension/tracebench/tests/test_tracebench.sh`
- Create: `extension/tracebench/output/.gitkeep`
- Create: `extension/tracebench/bpftrace/README.md`

### 4.2 修改根目录和文档

- Modify: `.gitignore`
- Modify: `README.md`
- Modify: `docs/FEATURES.md`
- Modify: `docs/DOC_AUDIT.md`
- Current document: `docs/PLANv2.md`
- Create/Modify: `docs/TRACEBENCH_REPORT_TEMPLATE.md`

### 4.3 不修改的路径

- Do not modify: `basic/scheduler/`
- Do not modify: `basic/memory/`
- Do not modify: `basic/sync/`
- Do not modify: `basic/filesystem/`
- Do not modify unless a later explicit task says so: `extension/oslab_monitor/`
- Do not modify for v2 root runner: `tests/run_all.sh`

`tests/run_all.sh` 继续只运行基础四模块测试；TraceBench 需要 Ubuntu VM、root 权限和 cgroup v2，因此单独运行 `extension/tracebench/tests/test_tracebench.sh`。

## 5. 模块职责与建议接口

### 5.1 `include/tracebench.h`

职责：

- 定义全局常量：`TB_NAME_LEN`、`TB_PATH_LEN`、`TB_LINE_LEN`、`TB_VALUE_LEN`。
- 定义枚举：`TbCommand`、`TbProfile`。
- 定义配置结构：`TbConfig`。
- 定义 cgroup 状态结构：`TbCgroup`。
- 定义 PSI 结构：`TbPsiLine`、`TbPsiResource`、`TbPsiSnapshot`。
- 定义 cgroup 指标结构：`TbCgroupStats`。
- 定义 oslab_monitor 结构：`TbOslabSnapshot`。
- 定义单行采样结构：`TbSample`。
- 声明各 `.c` 文件之间共享的函数。

建议函数名：

- `tb_parse_args`
- `tb_print_help`
- `tb_run_command`
- `tb_report_command`
- `tb_cleanup_command`
- `tb_cgroup_init`
- `tb_cgroup_create`
- `tb_cgroup_add_pid`
- `tb_cgroup_read_stats`
- `tb_cgroup_remove_run`
- `tb_cgroup_cleanup_all`
- `tb_run_workload_child`
- `tb_read_psi_snapshot`
- `tb_read_oslab_snapshot`
- `tb_write_csv_header`
- `tb_write_csv_sample`
- `tb_write_command_file`
- `tb_write_environment_file`
- `tb_write_summary_file`
- `tb_generate_markdown_report`
- `tb_mkdir_p`
- `tb_read_text_file`
- `tb_write_text_file`
- `tb_now_millis`
- `tb_print_error`

### 5.2 `src/main.c`

职责：

- 作为 `tracebench` CLI 入口。
- 调用 `tb_parse_args`。
- 分发 `--help`、`run`、`report`、`cleanup`。
- 保证错误场景返回非 `0`。
- 不实现 cgroup、workload、采样、报告细节。

### 5.3 `src/args.c`

职责：

- 解析 `tracebench --help`。
- 解析 `tracebench run`。
- 解析 `tracebench report`。
- 解析 `tracebench cleanup`。
- 填充默认参数。
- 校验 `--profile`、`--duration`、`--sample-interval`、`--output`、`--input`、`--cpu-workers`、`--memory-mb`、`--io-mb`、`--cgroup-name`。
- 确保 `--cgroup-name` 只允许字母、数字、下划线和短横线。
- 参数错误时输出 `error:`，不静默采用默认值。

### 5.4 `src/util.c`

职责：

- 文件读取和写入。
- 递归创建输出目录。
- 安全拼接路径。
- 单调时钟毫秒计算。
- 字符串转正整数。
- 字段查找和简单文本解析辅助。
- 统一 `error:` 输出。
- 检测当前是否 root。

### 5.5 `src/cgroup.c`

职责：

- 检测 cgroup v2 是否可用。
- 生成 run id：`YYYYMMDD_HHMMSS_<pid>`。
- 生成 cgroup 路径：`/sys/fs/cgroup/<cgroup_name>/<profile>/<run_id>/`。
- 创建 root、profile、run cgroup。
- 将 workload 子进程 PID 写入 `cgroup.procs`。
- 读取 `cpu.stat`、`memory.current`、`memory.events`。
- 删除本次 run cgroup。
- 执行全局 cleanup，但只清理 `/sys/fs/cgroup/<cgroup_name>/`。
- 不设置 `cpu.max`、`memory.max`、`memory.high`。

### 5.6 `src/workload.c`

职责：

- 实现 CPU workload：子进程创建 `--cpu-workers` 个 pthread 忙循环 worker。
- 实现 memory workload：分配并周期性触碰 `--memory-mb` 内存。
- 实现 I/O workload：写入 `<output>/tracebench_io.tmp`，写满 `--io-mb` 后执行 `fsync()`。
- 所有 workload 按 `duration` 自动结束。
- 所有 workload 只能由本次 `tracebench` 子进程启动，不依赖外部压测工具。

### 5.7 `src/sampler.c`

职责：

- 读取并解析 `/proc/pressure/cpu`。
- 读取并解析 `/proc/pressure/memory`。
- 读取并解析 `/proc/pressure/io`。
- 解析 `some` 和 `full` 行的 `avg10`、`avg60`、`avg300`、`total`。
- 如果某资源没有 `full` 行，对应字段输出 `NA`。
- 读取 `/proc/oslab_monitor/overview`。
- 解析 `total_tasks`、`running_tasks`、`sleeping_tasks`、`mem_free_kb`、`mem_available_kb`。
- 写入 `samples.csv` 表头和采样行。

### 5.8 `src/report.c`

职责：

- 生成 `summary.txt`。
- 读取 `samples.csv`。
- 对重点字段计算 `first`、`last`、`delta`、`max`。
- 生成 Markdown 报告。
- Markdown 报告固定包含 `PSI`、`cgroup`、`oslab_monitor` 和 `samples.csv` 路径。
- 对全列 `NA` 的字段输出“未采集”。

## 6. 固定 CSV 表头

`samples.csv` 第一行必须按以下顺序输出：

```text
sample_index
elapsed_ms
profile
duration_sec
sample_interval_sec
run_id
cgroup_enabled
cgroup_path
cpu_some_avg10
cpu_some_avg60
cpu_some_avg300
cpu_some_total
cpu_full_avg10
cpu_full_avg60
cpu_full_avg300
cpu_full_total
memory_some_avg10
memory_some_avg60
memory_some_avg300
memory_some_total
memory_full_avg10
memory_full_avg60
memory_full_avg300
memory_full_total
io_some_avg10
io_some_avg60
io_some_avg300
io_some_total
io_full_avg10
io_full_avg60
io_full_avg300
io_full_total
cgroup_cpu_usage_usec
cgroup_cpu_user_usec
cgroup_cpu_system_usec
cgroup_cpu_nr_periods
cgroup_cpu_nr_throttled
cgroup_cpu_throttled_usec
cgroup_memory_current
cgroup_memory_events_low
cgroup_memory_events_high
cgroup_memory_events_max
cgroup_memory_events_oom
cgroup_memory_events_oom_kill
cgroup_memory_events_oom_group_kill
oslab_monitor_available
oslab_total_tasks
oslab_running_tasks
oslab_sleeping_tasks
oslab_mem_free_kb
oslab_mem_available_kb
```

测试脚本必须检查最低字段和固定字段数量。新增字段必须同步 `docs/PRDv2.md`、`docs/TECHv2.md`、`docs/PLANv2.md` 和测试脚本。

## 7. 任务列表

### Task 1: TraceBench 骨架、P1 预留目录和忽略规则

**Files:**

- Create directory: `extension/tracebench/include/`

- Create directory: `extension/tracebench/src/`

- Create directory: `extension/tracebench/scripts/`

- Create directory: `extension/tracebench/tests/`

- Create directory: `extension/tracebench/output/`

- Create directory: `extension/tracebench/bpftrace/`

- Create: `extension/tracebench/output/.gitkeep`

- Create: `extension/tracebench/bpftrace/README.md`

- Modify: `.gitignore`

- [ ] **Step 1: 创建目录结构**

Run:

```bash
mkdir -p extension/tracebench/include
mkdir -p extension/tracebench/src
mkdir -p extension/tracebench/scripts
mkdir -p extension/tracebench/tests
mkdir -p extension/tracebench/output
mkdir -p extension/tracebench/bpftrace
```

Expected:

- 所有目录存在。

- 不创建 `test/` 单数目录。

- [ ] **Step 2: 创建 `output/.gitkeep`**

`extension/tracebench/output/.gitkeep` 职责：

- 保留输出目录。
- 不承载实验结果。

Expected:

- Git 可跟踪空输出目录。

- [ ] **Step 3: 创建 `bpftrace/README.md`**

`extension/tracebench/bpftrace/README.md` 必须说明：

- 本目录仅为 P1 可选 bpftrace 增强预留。
- P0 Makefile 和测试不依赖 bpftrace。
- 后续可添加 `sched_latency.bt` 和 `syscall_count.bt`。
- 未安装 bpftrace 时 P0 不失败。

Expected:

- P1 预留边界明确。

- 不创建 `.bt` 脚本作为 P0 必需文件。

- [ ] **Step 4: 追加 `.gitignore` 规则**

只能在 `.gitignore` 文件末尾追加以下规则，不能覆盖已有内容：

```text
extension/tracebench/tracebench
extension/tracebench/*.o
extension/tracebench/output/*/
!extension/tracebench/output/.gitkeep
```

Expected:

- `tracebench` 可执行文件和输出子目录不进入版本控制。

- `extension/tracebench/output/.gitkeep` 仍可提交。

- [ ] **Step 5: 验证骨架**

Run:

```bash
test -d extension/tracebench/include
test -d extension/tracebench/src
test -d extension/tracebench/scripts
test -d extension/tracebench/tests
test -d extension/tracebench/output
test -d extension/tracebench/bpftrace
test -f extension/tracebench/output/.gitkeep
test -f extension/tracebench/bpftrace/README.md
```

Expected:

- 所有命令返回 `0`。

- [ ] **Step 6: 建议提交**

Suggested commit:

```bash
git add .gitignore extension/tracebench/output/.gitkeep extension/tracebench/bpftrace/README.md
git commit -m "chore: create tracebench v2 skeleton"
```

**完成条件：**

- `extension/tracebench/` 骨架存在。
- `.gitignore` 只追加 TraceBench 产物规则。
- P1 bpftrace 明确为预留，不影响 P0。

### Task 2: 公共头文件、Makefile 和基础 CLI 参数解析

**Files:**

- Create: `extension/tracebench/Makefile`

- Create: `extension/tracebench/include/tracebench.h`

- Create: `extension/tracebench/src/main.c`

- Create: `extension/tracebench/src/args.c`

- Create: `extension/tracebench/src/util.c`

- Create: `extension/tracebench/tests/test_tracebench.sh`

- [ ] **Step 1: 编写 `Makefile` 职责**

`extension/tracebench/Makefile` 必须：

- 使用 `gcc`。
- 使用 `-Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude`。
- 使用 `-pthread` 链接。
- 编译 `src/main.c`、`src/args.c`、`src/cgroup.c`、`src/workload.c`、`src/sampler.c`、`src/report.c`、`src/util.c`。
- 生成可执行文件 `tracebench`。
- 支持 `make clean` 删除 `tracebench` 和 `*.o`。
- `make clean` 不删除 `output/` 中的实验结果。

Expected:

- 后续源码齐全后，`make` 能生成 `extension/tracebench/tracebench`。

- [ ] **Step 2: 编写 `tracebench.h` 职责**

`include/tracebench.h` 必须声明：

- 全局常量。
- `TbCommand`。
- `TbProfile`。
- `TbConfig`。
- `TbCgroup`。
- `TbPsiLine`、`TbPsiResource`、`TbPsiSnapshot`。
- `TbCgroupStats`。
- `TbOslabSnapshot`。
- `TbSample`。
- Task 5.1 中列出的建议函数名。

Expected:

- 所有 `.c` 文件只通过 `tracebench.h` 共享模块内类型和函数。

- 不创建跨项目公共库。

- [ ] **Step 3: 编写 `main.c` 职责**

`main.c` 必须：

- 调用 `tb_parse_args`。
- 对 `TB_CMD_HELP` 调用 `tb_print_help`。
- 对 `TB_CMD_RUN` 调用 `tb_run_command`。
- 对 `TB_CMD_REPORT` 调用 `tb_report_command`。
- 对 `TB_CMD_CLEANUP` 调用 `tb_cleanup_command`。
- 参数错误或执行错误时返回非 `0`。

Expected:

- `main.c` 不直接读取 PSI、cgroup 或 CSV。

- [ ] **Step 4: 编写 `args.c` 职责**

`args.c` 必须支持：

- `./tracebench --help`
- `./tracebench run --profile cpu|memory|io --duration N --sample-interval N --output PATH`
- `./tracebench report --input PATH --output PATH`
- `./tracebench cleanup`
- `--cpu-workers N`
- `--memory-mb N`
- `--io-mb N`
- `--cgroup-name NAME`
- `--no-cgroup`
- `--with-oslab-monitor`

参数默认值：

```text
cpu-workers = 2
memory-mb = 128
io-mb = 64
cgroup-name = oslab_tracebench
no-cgroup = false
with-oslab-monitor = false
```

错误场景：

- 未知子命令。
- 未知参数。
- 参数缺值。
- 非法 profile。
- `duration <= 0`。
- `sample-interval <= 0`。
- `sample-interval > duration`。
- `cpu-workers <= 0`。
- `memory-mb <= 0`。
- `io-mb <= 0`。
- `cgroup-name` 含非法字符。
- `run` 缺少 `--profile`、`--duration`、`--sample-interval` 或 `--output`。
- `report` 缺少 `--input` 或 `--output`。

Expected:

- 所有错误输出以 `error:` 开头。

- [ ] **Step 5: 编写 `util.c` 基础职责**

`util.c` 在本任务中至少提供：

- `tb_print_error`。
- 正整数解析辅助。
- cgroup 名称校验辅助。
- `tb_print_help`。

Expected:

- `--help` 输出包含 `run`、`report`、`cleanup`。

- [ ] **Step 6: 编写 CLI 测试脚本初版**

`extension/tracebench/tests/test_tracebench.sh` 初版必须：

- 使用 `#!/usr/bin/env bash`。
- 使用 `set -euo pipefail`。
- 包含中文文件头注释。
- 执行 `make`。
- 检查 `./tracebench --help` 输出包含 `run`、`report`、`cleanup`。
- 检查非法 profile 返回非 `0` 且输出包含 `error:`。
- 检查非法 duration 返回非 `0` 且输出包含 `error:`。
- 检查 `--sample-interval > --duration` 返回非 `0` 且输出包含 `error:`。

Expected:

- 当前阶段测试覆盖 CLI 基础行为。

- [ ] **Step 7: 运行 CLI 测试**

Run in Ubuntu VM or Linux build environment:

```bash
cd extension/tracebench
make
./tracebench --help
bash tests/test_tracebench.sh
make clean
```

Expected:

- `make` 成功。

- `--help` 输出包含 `run`、`report`、`cleanup`。

- 测试脚本返回 `0`。

- [ ] **Step 8: 建议提交**

Suggested commit:

```bash
git add extension/tracebench/Makefile extension/tracebench/include extension/tracebench/src extension/tracebench/tests/test_tracebench.sh
git commit -m "feat: add tracebench cli argument parsing"
```

**完成条件：**

- `tracebench --help` 可运行。
- 参数错误输出 `error:`。
- 基础 CLI 测试通过。

### Task 3: 输出目录、`command.txt` 和 `environment.txt`

**Files:**

- Modify: `extension/tracebench/include/tracebench.h`

- Modify: `extension/tracebench/src/main.c`

- Modify: `extension/tracebench/src/util.c`

- Modify: `extension/tracebench/src/report.c`

- Modify: `extension/tracebench/tests/test_tracebench.sh`

- [ ] **Step 1: 扩展 `util.c` 文件和目录辅助职责**

`util.c` 必须新增职责：

- 创建输出目录，目录不存在时自动创建。
- 允许输出目录已存在。
- 不删除输出目录中的其他用户文件。
- 安全拼接路径。
- 写入文本文件。
- 读取 `/etc/os-release` 或等价发行版信息。
- 获取 `uname -a`。
- 获取 `gcc --version` 第一行。
- 检测 `/proc/pressure/cpu`、`memory`、`io` 是否存在。
- 检测 `/proc/oslab_monitor/overview` 是否存在。

Expected:

- 文件写入失败时输出 `error:` 并返回非 `0`。

- [ ] **Step 2: 编写 `command.txt` 职责**

`run` 开始时必须写入：

```text
<output>/command.txt
```

内容必须包含：

- 完整命令。
- profile。
- duration_sec。
- sample_interval_sec。
- cpu_workers。
- memory_mb。
- io_mb。
- cgroup_name。
- no_cgroup。
- with_oslab_monitor。

Expected:

- 每次 run 覆盖旧的 `command.txt`。

- [ ] **Step 3: 编写 `environment.txt` 职责**

`run` 开始时必须写入：

```text
<output>/environment.txt
```

内容必须包含：

```text
uname:
os_release:
gcc:
cgroup_v2:
psi_cpu:
psi_memory:
psi_io:
oslab_monitor_overview:
tracebench_version:
```

Expected:

- `tracebench_version` 可以固定为 `v2-p0`。

- 若 PSI 文件缺失，`environment.txt` 记录缺失，同时默认 run 返回 `error:`。

- [ ] **Step 4: `run --no-cgroup` 低权限最小路径**

在本任务中，`run --no-cgroup` 可以先只完成：

- 创建输出目录。
- 写 `command.txt`。
- 写 `environment.txt`。
- 明确提示采样和 workload 尚由后续任务完成。

Expected:

- 如果实现阶段选择让 `run --no-cgroup` 在采样未实现前返回非 `0`，测试脚本不得把它作为完成项；完整成功检查放到 Task 9 之后。

- [ ] **Step 5: 扩展测试脚本检查输出文件**

`tests/test_tracebench.sh` 必须增加低权限检查：

```bash
./tracebench run --profile cpu --duration 1 --sample-interval 1 --output output/test_nocg --no-cgroup
test -f output/test_nocg/command.txt
test -f output/test_nocg/environment.txt
grep -q "profile: cpu" output/test_nocg/command.txt
grep -q "tracebench_version:" output/test_nocg/environment.txt
```

Expected:

- 该检查不要求 root。

- [ ] **Step 6: 运行测试**

Run:

```bash
cd extension/tracebench
make
bash tests/test_tracebench.sh
make clean
```

Expected:

- `command.txt` 和 `environment.txt` 生成。

- CLI 测试仍通过。

- [ ] **Step 7: 建议提交**

Suggested commit:

```bash
git add extension/tracebench
git commit -m "feat: write tracebench command and environment files"
```

**完成条件：**

- 输出目录创建规则明确。
- `command.txt` 和 `environment.txt` 生成并可被测试脚本检查。

### Task 4: PSI 解析与采样

**Files:**

- Modify: `extension/tracebench/include/tracebench.h`

- Modify: `extension/tracebench/src/sampler.c`

- Modify: `extension/tracebench/src/util.c`

- Modify: `extension/tracebench/tests/test_tracebench.sh`

- [ ] **Step 1: 编写 PSI 解析职责**

`sampler.c` 必须实现：

- 读取 `/proc/pressure/cpu`。
- 读取 `/proc/pressure/memory`。
- 读取 `/proc/pressure/io`。
- 解析 `some` 行的 `avg10`、`avg60`、`avg300`、`total`。
- 解析 `full` 行的 `avg10`、`avg60`、`avg300`、`total`。
- 若某资源没有 `full` 行，设置对应字段为缺失状态，后续 CSV 输出 `NA`。

Expected:

- PSI 文件缺失时默认 run 输出 `error:` 并返回非 `0`。

- [ ] **Step 2: 编写 PSI 测试输入设计**

测试脚本不依赖伪造 `/proc` 文件作为 P0 必需测试，但必须在真实环境中检查：

```bash
test -r /proc/pressure/cpu
test -r /proc/pressure/memory
test -r /proc/pressure/io
```

Expected:

- 如果文件不存在，测试脚本输出清晰错误，提示当前环境不满足 P0。

- [ ] **Step 3: 增加 `--no-cgroup` PSI 采样检查**

在 Task 9 完成 CSV 前，测试只需检查 PSI 读取路径可用；若当前实现已经能写 CSV，可检查表头字段：

```bash
grep -q "cpu_some_avg10" output/test_nocg/samples.csv
grep -q "memory_some_avg10" output/test_nocg/samples.csv
grep -q "io_some_avg10" output/test_nocg/samples.csv
```

Expected:

- PSI 字段名与 TECHv2 保持一致。

- [ ] **Step 4: 运行环境检查和测试**

Run in Ubuntu VM:

```bash
cd extension/tracebench
test -r /proc/pressure/cpu
test -r /proc/pressure/memory
test -r /proc/pressure/io
make
bash tests/test_tracebench.sh
make clean
```

Expected:

- PSI 路径存在。

- 测试脚本返回 `0`。

- [ ] **Step 5: 建议提交**

Suggested commit:

```bash
git add extension/tracebench
git commit -m "feat: add psi pressure parsing"
```

**完成条件：**

- PSI 三类资源读取和解析职责明确实现。
- 缺失 PSI 文件会导致 P0 run 失败。

### Task 5: cgroup v2 管理与指标读取

**Files:**

- Modify: `extension/tracebench/include/tracebench.h`

- Modify: `extension/tracebench/src/cgroup.c`

- Modify: `extension/tracebench/src/util.c`

- Modify: `extension/tracebench/tests/test_tracebench.sh`

- [ ] **Step 1: 编写 cgroup v2 检测职责**

`cgroup.c` 必须检测：

- `/sys/fs/cgroup` 存在。
- 当前系统启用 cgroup v2。
- 当前用户是否具备创建 cgroup 目录权限。
- 默认模式下 `geteuid() == 0`，否则输出 `error:`。

Expected:

- 非 root 默认 `run` 返回非 `0`，输出包含 `error:`。

- `--no-cgroup` 不执行写入性 cgroup 检测。

- [ ] **Step 2: 编写路径生成职责**

`cgroup.c` 必须生成：

```text
/sys/fs/cgroup/<cgroup_name>/<profile>/<run_id>/
```

其中：

- 默认 `cgroup_name = oslab_tracebench`。
- `profile = cpu|memory|io`。
- `run_id = YYYYMMDD_HHMMSS_<pid>`。

Expected:

- `cgroup_path` 后续写入 CSV。

- [ ] **Step 3: 编写 cgroup 创建和加入进程职责**

`cgroup.c` 必须：

- 创建 root cgroup。
- 创建 profile cgroup。
- 创建 run cgroup。
- 将 workload 子进程 PID 写入 run cgroup 的 `cgroup.procs`。

Expected:

- workload 开始前已经加入 run cgroup。

- [ ] **Step 4: 编写 cgroup 指标读取职责**

必须读取：

```text
cpu.stat
memory.current
memory.events
```

必须解析：

- `usage_usec`
- `user_usec`
- `system_usec`
- `nr_periods`
- `nr_throttled`
- `throttled_usec`
- `memory.current`
- `low`
- `high`
- `max`
- `oom`
- `oom_kill`
- `oom_group_kill`

Expected:

- 不存在的增强字段后续 CSV 输出 `NA`。

- `usage_usec`、`memory.current`、`memory.events` 最低字段缺失时默认 run 返回 `error:`。

- [ ] **Step 5: 编写单次 run cgroup 清理职责**

`cgroup.c` 必须：

- 实验结束后删除本次 run cgroup。
- 如果 profile/root cgroup 已为空，可以顺带删除。
- 如果 cgroup 仍忙，输出 warning，不杀死无法证明归属的进程。

Expected:

- 默认 run 结束后本次 run cgroup 不残留。

- [ ] **Step 6: 增加 cgroup 测试检查**

`tests/test_tracebench.sh` 必须包含：

```bash
if [ "$(id -u)" -eq 0 ]; then
  echo "running as root"
else
  echo "error: tracebench P0 integration test requires sudo/root" >&2
  exit 1
fi
```

并检查：

```bash
mount | grep -q cgroup2
test -d /sys/fs/cgroup
```

Expected:

- P0 集成测试明确要求 root。

- [ ] **Step 7: 运行 cgroup 测试**

Run in Ubuntu VM:

```bash
cd extension/tracebench
make
sudo bash tests/test_tracebench.sh
make clean
```

Expected:

- cgroup 环境检查通过。

- 需要完整 run 的检查可在 Task 9 后启用。

- [ ] **Step 8: 建议提交**

Suggested commit:

```bash
git add extension/tracebench
git commit -m "feat: add tracebench cgroup v2 management"
```

**完成条件：**

- cgroup v2 检测、创建、加入进程、指标读取和单次清理职责实现。
- 非 root 默认模式不会静默降级。

### Task 6: run 控制器、父子进程同步和 CPU workload

**Files:**

- Modify: `extension/tracebench/include/tracebench.h`

- Modify: `extension/tracebench/src/main.c`

- Modify: `extension/tracebench/src/workload.c`

- Modify: `extension/tracebench/src/sampler.c`

- Modify: `extension/tracebench/src/cgroup.c`

- Modify: `extension/tracebench/tests/test_tracebench.sh`

- [ ] **Step 1: 编写 `run` 控制器职责**

`tb_run_command` 必须按以下顺序执行：

1. 校验权限、参数和环境。
2. 创建输出目录。
3. 写 `command.txt`。
4. 写 `environment.txt`。
5. 如果启用 cgroup，创建 cgroup。
6. 创建 pipe。
7. `fork()` workload 子进程。
8. 子进程阻塞等待 pipe 启动信号。
9. 父进程将子进程 PID 写入 cgroup。
10. 父进程向 pipe 写入启动信号。
11. 父进程采样直到 `duration` 结束。
12. 父进程回收子进程。
13. 父进程执行本次 run 清理。

Expected:

- workload 压力主要发生在目标 cgroup 内。

- [ ] **Step 2: 编写 signal 清理职责**

`run` 必须处理：

- `SIGINT`
- `SIGTERM`

中断时：

- 只终止本次启动的 workload 子进程。
- `waitpid()` 回收子进程。
- 尽量删除本次 run cgroup。
- 尽量删除本次 I/O 临时文件。
- 返回非 `0`。

Expected:

- 不扫描和杀死系统中名字相似的其他进程。

- [ ] **Step 3: 编写 CPU workload 职责**

`workload.c` 必须实现 CPU profile：

- 子进程创建 `--cpu-workers` 个 pthread worker。
- 默认 worker 数为 `2`。
- worker 执行 CPU 密集型循环。
- worker 到达 `duration` 后退出。
- 子进程 join 所有 worker 后退出。

Expected:

- `duration=3` 的 CPU profile 应在 6 秒内结束。

- [ ] **Step 4: 扩展测试脚本检查 CPU profile**

`tests/test_tracebench.sh` 必须执行：

```bash
sudo ./tracebench run --profile cpu --duration 3 --sample-interval 1 --cpu-workers 2 --output output/test_cpu
test -f output/test_cpu/command.txt
test -f output/test_cpu/environment.txt
```

Expected:

- 命令返回 `0`。

- 输出目录存在。

- 后续 Task 9 再检查 `samples.csv` 内容。

- [ ] **Step 5: 检查默认非 root 失败**

测试脚本必须检查：

```bash
set +e
./tracebench run --profile cpu --duration 1 --sample-interval 1 --output output/non_root_cpu >output/non_root.out 2>&1
status=$?
set -e
test "$status" -ne 0
grep -q "error:" output/non_root.out
```

Expected:

- 非 root 默认模式不会静默降级。

- [ ] **Step 6: 运行 CPU profile 测试**

Run in Ubuntu VM:

```bash
cd extension/tracebench
make
sudo bash tests/test_tracebench.sh
make clean
```

Expected:

- CPU profile 成功运行并退出。

- 非 root 默认模式失败检查通过。

- [ ] **Step 7: 建议提交**

Suggested commit:

```bash
git add extension/tracebench
git commit -m "feat: add tracebench cpu workload run control"
```

**完成条件：**

- `run --profile cpu` 可运行。
- 父子进程同步规则实现。
- CPU workload 自动结束且可回收。

### Task 7: memory workload

**Files:**

- Modify: `extension/tracebench/include/tracebench.h`

- Modify: `extension/tracebench/src/workload.c`

- Modify: `extension/tracebench/src/args.c`

- Modify: `extension/tracebench/tests/test_tracebench.sh`

- [ ] **Step 1: 编写 memory 参数职责**

`args.c` 必须：

- 支持 `--memory-mb N`。
- 默认值为 `128`。
- 拒绝 `N <= 0`。
- 测试脚本使用 `64`，避免验收慢或占用过多 VM 内存。

Expected:

- 非法 `--memory-mb 0` 返回非 `0` 且输出 `error:`。

- [ ] **Step 2: 编写 memory workload 职责**

`workload.c` 必须实现 memory profile：

- 分配 `--memory-mb` 指定大小的内存。
- 按页大小步进写入，确保物理页实际映射。
- 实验期间周期性重新触碰页面。
- 不主动设置导致 OOM 的 cgroup 限制。
- 到达 `duration` 后释放内存并退出。

Expected:

- 默认不触发 OOM。

- `duration=3 --memory-mb 64` 应在 6 秒内结束。

- [ ] **Step 3: 扩展测试脚本检查 memory profile**

`tests/test_tracebench.sh` 必须执行：

```bash
sudo ./tracebench run --profile memory --duration 3 --sample-interval 1 --memory-mb 64 --output output/test_memory
test -f output/test_memory/command.txt
test -f output/test_memory/environment.txt
```

Expected:

- 命令返回 `0`。

- 输出目录存在。

- [ ] **Step 4: 增加非法 memory 参数测试**

测试脚本必须检查：

```bash
set +e
./tracebench run --profile memory --duration 1 --sample-interval 1 --memory-mb 0 --output output/bad_memory >output/bad_memory.out 2>&1
status=$?
set -e
test "$status" -ne 0
grep -q "error:" output/bad_memory.out
```

Expected:

- 参数错误被明确拒绝。

- [ ] **Step 5: 运行 memory profile 测试**

Run in Ubuntu VM:

```bash
cd extension/tracebench
make
sudo bash tests/test_tracebench.sh
make clean
```

Expected:

- CPU 和 memory profile 均通过。

- [ ] **Step 6: 建议提交**

Suggested commit:

```bash
git add extension/tracebench
git commit -m "feat: add tracebench memory workload"
```

**完成条件：**

- `run --profile memory` 可运行。
- memory workload 自动结束。
- 非法内存参数有 `error:`。

### Task 8: I/O workload 和临时文件清理

**Files:**

- Modify: `extension/tracebench/include/tracebench.h`

- Modify: `extension/tracebench/src/workload.c`

- Modify: `extension/tracebench/src/args.c`

- Modify: `extension/tracebench/src/util.c`

- Modify: `extension/tracebench/tests/test_tracebench.sh`

- [ ] **Step 1: 编写 I/O 参数职责**

`args.c` 必须：

- 支持 `--io-mb N`。
- 默认值为 `64`。
- 拒绝 `N <= 0`。
- 测试脚本使用 `16`，避免验收慢和磁盘压力过大。

Expected:

- 非法 `--io-mb 0` 返回非 `0` 且输出 `error:`。

- [ ] **Step 2: 编写 I/O workload 职责**

`workload.c` 必须实现 I/O profile：

- 临时文件路径固定为 `<output>/tracebench_io.tmp`。
- 使用固定大小缓冲区写入。
- 写满 `--io-mb` 后执行 `fsync()`。
- 若时间未到，可覆盖同一临时文件继续制造 I/O。
- 正常结束时关闭文件。
- 正常结束后删除 `tracebench_io.tmp`。

Expected:

- 不无限增长磁盘使用。

- `duration=3 --io-mb 16` 应在合理时间内结束。

- [ ] **Step 3: 扩展测试脚本检查 I/O profile**

`tests/test_tracebench.sh` 必须执行：

```bash
sudo ./tracebench run --profile io --duration 3 --sample-interval 1 --io-mb 16 --output output/test_io
test -f output/test_io/command.txt
test -f output/test_io/environment.txt
test ! -f output/test_io/tracebench_io.tmp
```

Expected:

- 正常结束后 I/O 临时文件不存在。

- [ ] **Step 4: 增加非法 I/O 参数测试**

测试脚本必须检查：

```bash
set +e
./tracebench run --profile io --duration 1 --sample-interval 1 --io-mb 0 --output output/bad_io >output/bad_io.out 2>&1
status=$?
set -e
test "$status" -ne 0
grep -q "error:" output/bad_io.out
```

Expected:

- 参数错误被明确拒绝。

- [ ] **Step 5: 运行 I/O profile 测试**

Run in Ubuntu VM:

```bash
cd extension/tracebench
make
sudo bash tests/test_tracebench.sh
make clean
```

Expected:

- CPU、memory、io 三类 profile 均可运行。

- I/O 临时文件正常清理。

- [ ] **Step 6: 建议提交**

Suggested commit:

```bash
git add extension/tracebench
git commit -m "feat: add tracebench io workload"
```

**完成条件：**

- `run --profile io` 可运行。
- I/O 临时文件写入、fsync 和清理规则实现。
- 非法 I/O 参数有 `error:`。

### Task 9: 周期采样和 `samples.csv`

**Files:**

- Modify: `extension/tracebench/include/tracebench.h`

- Modify: `extension/tracebench/src/sampler.c`

- Modify: `extension/tracebench/src/main.c`

- Modify: `extension/tracebench/src/report.c`

- Modify: `extension/tracebench/tests/test_tracebench.sh`

- [ ] **Step 1: 编写 CSV 表头职责**

`sampler.c` 必须写入：

```text
<output>/samples.csv
```

CSV 第一行必须严格等于本计划第 6 章固定字段顺序。

Expected:

- 测试脚本可以用字段名和字段数量稳定检查。

- [ ] **Step 2: 编写采样行职责**

每行必须包含：

- `sample_index`，从 `0` 开始。
- `elapsed_ms`，使用单调时钟。
- profile。
- duration。
- sample interval。
- run id。
- cgroup 是否启用。
- cgroup 路径或 `NA`。
- PSI 字段。
- cgroup 字段或 `NA`。
- oslab_monitor 字段或 `NA`。

Expected:

- 布尔值使用 `true` 或 `false`。

- 缺失字段使用 `NA`。

- 每行字段数量与表头一致。

- [ ] **Step 3: 编写采样循环职责**

采样循环必须：

- 按 `sample-interval` 周期采样。
- `duration=5 sample-interval=1` 至少生成 5 行。
- `duration=3 sample-interval=1` 至少生成 3 行。
- 不强制末尾额外第 `duration` 秒采样。

Expected:

- 测试不依赖机器精确调度时间。

- [ ] **Step 4: 扩展测试脚本检查 CSV 表头**

`tests/test_tracebench.sh` 必须检查三个 profile 的 `samples.csv`：

```bash
test -f output/test_cpu/samples.csv
test -f output/test_memory/samples.csv
test -f output/test_io/samples.csv
grep -q "cpu_some_avg10" output/test_cpu/samples.csv
grep -q "memory_some_avg10" output/test_memory/samples.csv
grep -q "io_some_avg10" output/test_io/samples.csv
grep -q "cgroup_cpu_usage_usec" output/test_cpu/samples.csv
grep -q "cgroup_memory_current" output/test_memory/samples.csv
grep -q "oslab_monitor_available" output/test_cpu/samples.csv
```

Expected:

- P0 最低字段均存在。

- [ ] **Step 5: 扩展测试脚本检查行数**

测试脚本必须检查：

```bash
cpu_rows=$(($(wc -l < output/test_cpu/samples.csv) - 1))
memory_rows=$(($(wc -l < output/test_memory/samples.csv) - 1))
io_rows=$(($(wc -l < output/test_io/samples.csv) - 1))
test "$cpu_rows" -ge 3
test "$memory_rows" -ge 3
test "$io_rows" -ge 3
```

Expected:

- 每个 `duration=3 sample-interval=1` 的 profile 至少 3 行采样。

- [ ] **Step 6: 扩展测试脚本检查字段数量一致**

测试脚本必须对每个 CSV 检查：

```bash
awk -F',' 'NR==1 {n=NF} NR>1 && NF!=n {exit 1}' output/test_cpu/samples.csv
awk -F',' 'NR==1 {n=NF} NR>1 && NF!=n {exit 1}' output/test_memory/samples.csv
awk -F',' 'NR==1 {n=NF} NR>1 && NF!=n {exit 1}' output/test_io/samples.csv
```

Expected:

- 每行字段数量一致。

- [ ] **Step 7: 扩展 `--no-cgroup` CSV 测试**

测试脚本必须执行：

```bash
./tracebench run --profile cpu --duration 2 --sample-interval 1 --cpu-workers 1 --output output/test_nocg --no-cgroup
test -f output/test_nocg/samples.csv
grep -q "cgroup_enabled" output/test_nocg/samples.csv
grep -q "false" output/test_nocg/samples.csv
grep -q "NA" output/test_nocg/samples.csv
```

Expected:

- 低权限演示可运行。

- cgroup 字段为 `NA`。

- 该检查不替代 sudo P0 测试。

- [ ] **Step 8: 运行 CSV 测试**

Run in Ubuntu VM:

```bash
cd extension/tracebench
make
sudo bash tests/test_tracebench.sh
make clean
```

Expected:

- 三个 profile 都生成 `samples.csv`。

- 表头、行数、字段数量检查通过。

- [ ] **Step 9: 建议提交**

Suggested commit:

```bash
git add extension/tracebench
git commit -m "feat: add tracebench csv sampling output"
```

**完成条件：**

- `samples.csv` 稳定生成。
- PSI、cgroup 和 oslab_monitor 可用性字段进入 CSV。
- `--no-cgroup` CSV 语义明确。

### Task 10: oslab_monitor 对照采样

**Files:**

- Modify: `extension/tracebench/include/tracebench.h`

- Modify: `extension/tracebench/src/sampler.c`

- Modify: `extension/tracebench/src/args.c`

- Modify: `extension/tracebench/tests/test_tracebench.sh`

- [ ] **Step 1: 编写 oslab_monitor 默认采样职责**

`sampler.c` 必须读取：

```text
/proc/oslab_monitor/overview
```

如果文件存在，解析：

- `total_tasks`
- `running_tasks`
- `sleeping_tasks`
- `mem_free_kb`
- `mem_available_kb`

Expected:

- 文件存在但字段缺失时，缺失字段输出 `NA`。

- 文件不存在时默认不报错。

- [ ] **Step 2: 编写 `--with-oslab-monitor` 职责**

`args.c` 和 `run` 必须支持：

```bash
--with-oslab-monitor
```

规则：

- 指定该参数时，`/proc/oslab_monitor/overview` 必须存在且可读。
- 不存在或不可读时输出 `error:` 并返回非 `0`。
- 默认未指定该参数时，模块未加载不失败。

Expected:

- 该参数只控制 oslab_monitor 是否强制可用，不影响 PSI 和 cgroup 采样。

- [ ] **Step 3: 扩展默认未加载模块测试**

测试脚本必须检查：

```bash
grep -q "oslab_monitor_available" output/test_cpu/samples.csv
```

若 `/proc/oslab_monitor/overview` 不存在，还必须检查：

```bash
grep -q "false" output/test_cpu/samples.csv
```

Expected:

- 未加载 oslab_monitor 时默认 run 不失败。

- [ ] **Step 4: 扩展 `--with-oslab-monitor` 错误测试**

如果 `/proc/oslab_monitor/overview` 不存在，测试脚本必须执行：

```bash
set +e
sudo ./tracebench run --profile cpu --duration 1 --sample-interval 1 --output output/with_om --with-oslab-monitor >output/with_om.out 2>&1
status=$?
set -e
test "$status" -ne 0
grep -q "error:" output/with_om.out
```

Expected:

- 模块未加载且用户强制要求时返回非 `0`。

- [ ] **Step 5: 已加载模块时的条件增强检查**

如果测试开始时 `/proc/oslab_monitor/overview` 存在，测试脚本必须额外检查：

```bash
grep -q "oslab_total_tasks" output/test_cpu/samples.csv
```

Expected:

- 条件增强不要求测试脚本自动加载内核模块。

- [ ] **Step 6: 运行 oslab_monitor 测试**

Run in Ubuntu VM:

```bash
cd extension/tracebench
make
sudo bash tests/test_tracebench.sh
make clean
```

Expected:

- 默认未加载模块不失败。

- `--with-oslab-monitor` 语义符合 TECHv2。

- [ ] **Step 7: 建议提交**

Suggested commit:

```bash
git add extension/tracebench
git commit -m "feat: add oslab monitor overview sampling"
```

**完成条件：**

- oslab_monitor 对照字段进入 CSV。
- 默认可选，显式要求时强制。

### Task 11: `summary.txt` 和 Markdown report

**Files:**

- Modify: `extension/tracebench/include/tracebench.h`

- Modify: `extension/tracebench/src/report.c`

- Modify: `extension/tracebench/src/main.c`

- Modify: `extension/tracebench/src/args.c`

- Modify: `extension/tracebench/tests/test_tracebench.sh`

- [ ] **Step 1: 编写 `summary.txt` 职责**

`run` 结束时必须生成：

```text
<output>/summary.txt
```

内容必须包含：

- profile。
- duration。
- sample interval。
- `samples.csv` 路径。
- 采样行数。
- cgroup 是否启用。
- oslab_monitor 是否可用。
- 重点字段的 `first`、`last`、`delta`、`max`。
- 如果使用 `--no-cgroup`，说明不满足 P0 完整验收。
- 如果清理 cgroup 或 I/O 临时文件失败，说明失败原因。

Expected:

- `summary.txt` 在 `run` 结束时生成，不依赖用户再执行 `report`。

- [ ] **Step 2: 编写摘要统计职责**

`report.c` 必须对以下字段计算 `first`、`last`、`delta`、`max`：

- `cpu_some_total`
- `memory_some_total`
- `io_some_total`
- `cgroup_cpu_usage_usec`
- `cgroup_memory_current`
- `cgroup_memory_events_high`
- `cgroup_memory_events_max`
- `cgroup_memory_events_oom`
- `oslab_total_tasks`
- `oslab_running_tasks`
- `oslab_mem_free_kb`
- `oslab_mem_available_kb`

Expected:

- `NA` 字段统计时跳过。

- 全列 `NA` 时输出“未采集”。

- [ ] **Step 3: 编写 `report` 子命令职责**

`tracebench report` 必须支持：

```bash
./tracebench report --input output/test_cpu --output output/test_cpu/report.md
```

行为：

- 读取 `<input>/samples.csv`。
- 生成 Markdown 文件。
- 缺少 `samples.csv` 时输出 `error:` 并返回非 `0`。

Expected:

- `report` 不需要 root。

- [ ] **Step 4: Markdown 报告结构**

Markdown 报告必须包含：

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

并必须出现关键词：

- `PSI`
- `cgroup`
- `oslab_monitor`
- `samples.csv`

Expected:

- 报告可直接复制到课程报告。

- [ ] **Step 5: 扩展测试脚本检查 summary 和 report**

测试脚本必须执行：

```bash
test -f output/test_cpu/summary.txt
grep -q "profile" output/test_cpu/summary.txt
grep -q "samples.csv" output/test_cpu/summary.txt
./tracebench report --input output/test_cpu --output output/test_cpu/report.md
test -f output/test_cpu/report.md
grep -q "PSI" output/test_cpu/report.md
grep -q "cgroup" output/test_cpu/report.md
grep -q "oslab_monitor" output/test_cpu/report.md
grep -q "samples.csv" output/test_cpu/report.md
```

Expected:

- summary 和 Markdown 报告均生成。

- [ ] **Step 6: 增加 report 错误测试**

测试脚本必须检查：

```bash
set +e
./tracebench report --input output/missing_report_input --output output/missing.md >output/missing_report.out 2>&1
status=$?
set -e
test "$status" -ne 0
grep -q "error:" output/missing_report.out
```

Expected:

- 缺少 `samples.csv` 时明确失败。

- [ ] **Step 7: 运行 report 测试**

Run in Ubuntu VM:

```bash
cd extension/tracebench
make
sudo bash tests/test_tracebench.sh
make clean
```

Expected:

- summary 和 report 检查通过。

- [ ] **Step 8: 建议提交**

Suggested commit:

```bash
git add extension/tracebench
git commit -m "feat: add tracebench summary and markdown report"
```

**完成条件：**

- `summary.txt` 在 run 后生成。
- `tracebench report` 能生成 Markdown。
- 报告内容满足 PRDv2 和 TECHv2。

### Task 12: `tracebench cleanup`

**Files:**

- Modify: `extension/tracebench/include/tracebench.h`

- Modify: `extension/tracebench/src/cgroup.c`

- Modify: `extension/tracebench/src/main.c`

- Modify: `extension/tracebench/src/util.c`

- Modify: `extension/tracebench/tests/test_tracebench.sh`

- [ ] **Step 1: 编写 cleanup 命令职责**

`tracebench cleanup` 必须：

- 默认使用 `cgroup_name = oslab_tracebench`。
- 支持 `--cgroup-name NAME`。
- 默认要求 root。
- 权限不足时输出 `error:` 并返回非 `0`。

Expected:

- `cleanup` 不调用 `sudo`。

- [ ] **Step 2: 编写 cgroup cleanup 职责**

cleanup 只清理：

```text
/sys/fs/cgroup/<cgroup_name>/
```

规则：

- 自底向上删除空 cgroup。
- 如果 cgroup 中仍有进程，停止删除该 cgroup 并输出明确提示。
- 不杀死无法证明归属 TraceBench 当前运行的进程。

Expected:

- 不影响其他 cgroup。

- [ ] **Step 3: 编写 I/O 临时文件 cleanup 职责**

cleanup 只扫描：

```text
extension/tracebench/output/*/tracebench_io.tmp
```

规则：

- 只删除文件名精确为 `tracebench_io.tmp` 的文件。
- 不删除 `samples.csv`。
- 不删除 `summary.txt`。
- 不删除 Markdown 报告。
- 不删除输出目录。

Expected:

- cleanup 不造成实验材料丢失。

- [ ] **Step 4: 扩展测试脚本检查 cleanup**

测试脚本必须执行：

```bash
sudo ./tracebench cleanup
test -f output/test_cpu/samples.csv
test -f output/test_cpu/summary.txt
test -f output/test_cpu/report.md
test ! -f output/test_io/tracebench_io.tmp
```

Expected:

- cleanup 不删除 CSV 和报告。

- I/O 临时文件不存在。

- [ ] **Step 5: 增加 cleanup 非 root 错误测试**

测试脚本必须检查：

```bash
set +e
./tracebench cleanup >output/cleanup_non_root.out 2>&1
status=$?
set -e
test "$status" -ne 0
grep -q "error:" output/cleanup_non_root.out
```

Expected:

- 非 root cleanup 返回非 `0`。

- [ ] **Step 6: 运行 cleanup 测试**

Run in Ubuntu VM:

```bash
cd extension/tracebench
make
sudo bash tests/test_tracebench.sh
make clean
```

Expected:

- cleanup 测试通过。

- [ ] **Step 7: 建议提交**

Suggested commit:

```bash
git add extension/tracebench
git commit -m "feat: add tracebench cleanup command"
```

**完成条件：**

- cleanup 边界与 TECHv2 一致。
- 不删除实验 CSV 和报告。
- 非 root 错误语义明确。

### Task 13: Demo scripts

**Files:**

- Create: `extension/tracebench/scripts/run_cpu_demo.sh`

- Create: `extension/tracebench/scripts/run_memory_demo.sh`

- Create: `extension/tracebench/scripts/run_io_demo.sh`

- Create: `extension/tracebench/scripts/cleanup.sh`

- Modify: `extension/tracebench/tests/test_tracebench.sh`

- [ ] **Step 1: 编写 `run_cpu_demo.sh` 职责**

脚本必须：

- 使用 `#!/usr/bin/env bash`。
- 使用 `set -euo pipefail`。
- 包含中文文件头注释。
- 进入 `extension/tracebench/`。
- 执行 `make`。
- 执行：

```bash
sudo ./tracebench run --profile cpu --duration 5 --sample-interval 1 --cpu-workers 2 --output output/cpu
./tracebench report --input output/cpu --output output/cpu/report.md
```

Expected:

- 生成 `output/cpu/samples.csv` 和 `output/cpu/report.md`。

- [ ] **Step 2: 编写 `run_memory_demo.sh` 职责**

脚本必须执行：

```bash
sudo ./tracebench run --profile memory --duration 5 --sample-interval 1 --memory-mb 128 --output output/memory
./tracebench report --input output/memory --output output/memory/report.md
```

Expected:

- 生成 `output/memory/samples.csv` 和 `output/memory/report.md`。

- [ ] **Step 3: 编写 `run_io_demo.sh` 职责**

脚本必须执行：

```bash
sudo ./tracebench run --profile io --duration 5 --sample-interval 1 --io-mb 64 --output output/io
./tracebench report --input output/io --output output/io/report.md
```

Expected:

- 生成 `output/io/samples.csv` 和 `output/io/report.md`。

- [ ] **Step 4: 编写 `cleanup.sh` 职责**

脚本必须：

- 进入 `extension/tracebench/`。
- 执行 `make`。
- 执行 `sudo ./tracebench cleanup`。
- 明确说明 cleanup 不删除 CSV 和报告。

Expected:

- 清理 cgroup 和残留 I/O 临时文件。

- [ ] **Step 5: Bash 语法检查**

Run:

```bash
cd extension/tracebench
bash -n scripts/run_cpu_demo.sh
bash -n scripts/run_memory_demo.sh
bash -n scripts/run_io_demo.sh
bash -n scripts/cleanup.sh
```

Expected:

- 所有脚本语法检查通过。

- [ ] **Step 6: 扩展测试脚本检查 demo 脚本存在和语法**

`tests/test_tracebench.sh` 必须检查：

```bash
bash -n scripts/run_cpu_demo.sh
bash -n scripts/run_memory_demo.sh
bash -n scripts/run_io_demo.sh
bash -n scripts/cleanup.sh
```

Expected:

- 集成测试覆盖 demo 脚本语法。

- [ ] **Step 7: 建议提交**

Suggested commit:

```bash
git add extension/tracebench/scripts extension/tracebench/tests/test_tracebench.sh
git commit -m "test: add tracebench demo scripts"
```

**完成条件：**

- 三类 profile 均有演示脚本。
- cleanup 有演示脚本。
- 脚本语法通过。

### Task 14: P0 集成测试定稿与 Ubuntu VM 验证

**Files:**

- Modify: `extension/tracebench/tests/test_tracebench.sh`

- [ ] **Step 1: 定稿测试脚本结构**

`test_tracebench.sh` 必须包含：

- `#!/usr/bin/env bash`
- 中文文件头注释。
- `set -euo pipefail`
- `cleanup` 函数。
- `trap cleanup EXIT`

`cleanup` 函数必须：

- 尝试运行 `sudo ./tracebench cleanup`。
- 不删除 `samples.csv` 和报告。
- 允许 cleanup 在环境不完整时失败但不掩盖原始测试失败。

Expected:

- 测试中途失败时尽量清理 cgroup 和临时文件。

- [ ] **Step 2: 定稿环境检查**

测试脚本必须检查：

```bash
test -r /proc/pressure/cpu
test -r /proc/pressure/memory
test -r /proc/pressure/io
mount | grep -q cgroup2
```

Expected:

- 不满足 P0 环境时测试明确失败。

- [ ] **Step 3: 定稿构建检查**

测试脚本必须执行：

```bash
make clean
make
test -x ./tracebench
./tracebench --help | grep -q "run"
./tracebench --help | grep -q "report"
./tracebench --help | grep -q "cleanup"
```

Expected:

- 构建和帮助输出通过。

- [ ] **Step 4: 定稿参数错误检查**

测试脚本必须检查以下错误均返回非 `0` 且输出 `error:`：

- 非法 profile。
- `duration = 0`。
- `sample-interval > duration`。
- `memory-mb = 0`。
- `io-mb = 0`。
- 非 root 默认 run。
- 非 root cleanup。

Expected:

- 参数和权限错误都可验收。

- [ ] **Step 5: 定稿三类 profile 检查**

测试脚本必须执行：

```bash
sudo ./tracebench run --profile cpu --duration 3 --sample-interval 1 --cpu-workers 2 --output output/test_cpu
sudo ./tracebench run --profile memory --duration 3 --sample-interval 1 --memory-mb 64 --output output/test_memory
sudo ./tracebench run --profile io --duration 3 --sample-interval 1 --io-mb 16 --output output/test_io
```

Expected:

- 每条命令返回 `0`。

- 每个输出目录包含 `command.txt`、`environment.txt`、`samples.csv`、`summary.txt`。

- I/O 临时文件正常删除。

- [ ] **Step 6: 定稿 CSV 检查**

测试脚本必须对每个 profile 检查：

- 表头包含 PSI 字段。
- 表头包含 cgroup 字段。
- 表头包含 oslab_monitor 字段。
- 采样行数不少于 3。
- 每行字段数量一致。

Expected:

- CSV 可用于课程报告。

- [ ] **Step 7: 定稿 report 检查**

测试脚本必须执行：

```bash
./tracebench report --input output/test_cpu --output output/test_cpu/report.md
test -f output/test_cpu/report.md
grep -q "PSI" output/test_cpu/report.md
grep -q "cgroup" output/test_cpu/report.md
grep -q "oslab_monitor" output/test_cpu/report.md
grep -q "samples.csv" output/test_cpu/report.md
```

Expected:

- Markdown 报告生成并包含关键内容。

- [ ] **Step 8: 定稿 `--no-cgroup` 检查**

测试脚本必须执行：

```bash
./tracebench run --profile cpu --duration 2 --sample-interval 1 --cpu-workers 1 --output output/test_nocg --no-cgroup
test -f output/test_nocg/samples.csv
grep -q "cgroup_enabled" output/test_nocg/samples.csv
grep -q "false" output/test_nocg/samples.csv
grep -q "NA" output/test_nocg/samples.csv
```

Expected:

- 低权限演示可运行。

- 报告和 summary 必须说明不满足 P0 完整验收。

- [ ] **Step 9: 定稿 oslab_monitor 检查**

测试脚本必须：

- 不自动加载 `oslab_monitor.ko`。
- 默认 run 未加载模块时不失败。
- 未加载模块且指定 `--with-oslab-monitor` 时返回非 `0` 并输出 `error:`。
- 如果 `/proc/oslab_monitor/overview` 已存在，则额外检查 CSV 包含 oslab 字段。

Expected:

- TraceBench 不强依赖内核模块。

- [ ] **Step 10: 定稿 cleanup 检查**

测试脚本必须执行：

```bash
sudo ./tracebench cleanup
test -f output/test_cpu/samples.csv
test -f output/test_cpu/report.md
test ! -f output/test_io/tracebench_io.tmp
```

Expected:

- cleanup 不删除实验材料。

- [ ] **Step 11: 运行完整 P0 集成测试**

Run in Ubuntu VM:

```bash
cd extension/tracebench
sudo bash tests/test_tracebench.sh
```

Expected:

- 测试脚本返回 `0`。

- 输出说明 CPU、memory、io、CSV、report、cleanup 均通过。

- [ ] **Step 12: 建议提交**

Suggested commit:

```bash
git add extension/tracebench/tests/test_tracebench.sh
git commit -m "test: finalize tracebench p0 integration test"
```

**完成条件：**

- P0 Bash 集成测试覆盖 PRDv2 最终验收清单。
- Ubuntu VM 中 `sudo bash tests/test_tracebench.sh` 通过。

### Task 15: 文档同步、报告模板和最终验收记录

**Files:**

- Modify: `README.md`

- Modify: `docs/FEATURES.md`

- Modify: `docs/DOC_AUDIT.md`

- Create/Modify: `docs/TRACEBENCH_REPORT_TEMPLATE.md`

- Modify: `docs/PLANv2.md` if implementation changes planned filenames, commands, or tests

- [ ] **Step 1: 同步 README 职责**

`README.md` 必须新增 v2 TraceBench 说明：

- v2 仍处于新增扩展模块，不替代基础四模块和 oslab_monitor。
- P0 环境：Ubuntu 24.04 LTS VM、cgroup v2、`/proc/pressure/*`。
- TraceBench 构建命令：

```bash
cd extension/tracebench
make
./tracebench --help
```

- CPU、memory、io 运行命令：

```bash
sudo ./tracebench run --profile cpu --duration 3 --sample-interval 1 --cpu-workers 2 --output output/test_cpu
sudo ./tracebench run --profile memory --duration 3 --sample-interval 1 --memory-mb 64 --output output/test_memory
sudo ./tracebench run --profile io --duration 3 --sample-interval 1 --io-mb 16 --output output/test_io
```

- 报告命令：

```bash
./tracebench report --input output/test_cpu --output output/test_cpu/report.md
```

- cleanup 命令：

```bash
sudo ./tracebench cleanup
```

- P0 集成测试命令：

```bash
sudo bash tests/test_tracebench.sh
```

Expected:

- README 中 v2 命令与 PLANv2、TECHv2 一致。

- [ ] **Step 2: 同步 FEATURES 职责**

`docs/FEATURES.md` 必须新增 v2 用户可见功能：

- `tracebench --help`。
- `tracebench run --profile cpu`。
- `tracebench run --profile memory`。
- `tracebench run --profile io`。
- cgroup v2 采样。
- PSI 采样。
- oslab_monitor 对照采样。
- `samples.csv`。
- `summary.txt`。
- Markdown report。
- `tracebench cleanup`。
- P1/P2 为可选增强，不纳入默认验收。

Expected:

- FEATURES 与 PRDv2/TECHv2/PLANv2 一致。

- [ ] **Step 3: 同步 DOC_AUDIT 职责**

`docs/DOC_AUDIT.md` 必须记录：

- 新增 `docs/PRDv2.md`。
- 新增 `docs/TECHv2.md`。
- 新增 `docs/PLANv2.md`。
- 新增 `extension/tracebench/` 模块。
- 新增 `extension/tracebench/tests/test_tracebench.sh`。
- 根测试 `tests/run_all.sh` 不纳入 TraceBench，因为 TraceBench 需要 root、cgroup v2 和 Ubuntu VM。
- 尚未或已经验证的 Ubuntu VM 环境。

Expected:

- 文档审计能说明 v2 文档和代码结构是否一致。

- [ ] **Step 4: 创建报告模板**

`docs/TRACEBENCH_REPORT_TEMPLATE.md` 必须包含：

```markdown
# OSLab TraceBench v2 实验报告模板

## 1. 实验配置
## 2. 运行环境
## 3. 采样文件
## 4. PSI 资源压力摘要
## 5. cgroup 指标摘要
## 6. oslab_monitor 对照结果
## 7. 实验现象分析
## 8. 局限性
## 9. 可选增强说明
```

并说明：

- `samples.csv` 来自 `tracebench run`。
- Markdown 报告可由 `tracebench report` 生成。
- P1 tracefs/bpftrace 和 P2 sched_ext 如未启用，应在局限性或可选增强说明中记录原因。

Expected:

- 报告模板覆盖 PRDv2 报告材料需求。

- [ ] **Step 5: 运行文档一致性检查**

Run:

```bash
rg -n "tracebench|PRDv2|TECHv2|PLANv2|samples.csv|cgroup|PSI|oslab_monitor" README.md docs/FEATURES.md docs/DOC_AUDIT.md docs/TRACEBENCH_REPORT_TEMPLATE.md
rg -n "0644[[:space:]]+或[[:space:]]+0666" README.md docs
```

Expected:

- 不存在过时 `/proc` 权限表述。

- README、FEATURES、DOC_AUDIT、报告模板均能找到 v2 关键内容。

- [ ] **Step 6: 运行最终验收命令**

Run in Ubuntu VM:

```bash
cd extension/tracebench
make
sudo bash tests/test_tracebench.sh
make clean
```

Expected:

- `make` 成功。

- P0 集成测试通过。

- `make clean` 清理编译产物，不删除输出报告材料。

- [ ] **Step 7: 建议提交**

Suggested commit:

```bash
git add README.md docs/FEATURES.md docs/DOC_AUDIT.md docs/TRACEBENCH_REPORT_TEMPLATE.md docs/PLANv2.md
git commit -m "docs: add tracebench v2 implementation plan and docs"
```

**完成条件：**

- README、FEATURES、DOC_AUDIT 与 v2 实现一致。
- 报告模板存在。
- 文档中无未决占位词。

## 8. P1/P2 预留说明

P1/P2 不纳入 P0 默认实现任务。后续如果时间充足，可以在 P0 完成后另开计划。

### 8.1 P1 tracefs

后续可新增任务：

- 检测 `/sys/kernel/tracing` 和 `/sys/kernel/debug/tracing`。
- 检测 `events/sched/sched_switch` 和 `events/sched/sched_wakeup`。
- 记录启用前状态。
- 实验期间启用本次需要的事件。
- 实验结束后恢复状态。
- tracefs 不可用时在报告中说明，不影响 P0。

### 8.2 P1 bpftrace

后续可新增文件：

- `extension/tracebench/bpftrace/sched_latency.bt`
- `extension/tracebench/bpftrace/syscall_count.bt`

约束：

- P0 Makefile 不依赖 bpftrace。
- P0 测试不要求 bpftrace。
- bpftrace 不存在时跳过并在报告中说明。

### 8.3 P2 sched_ext

后续可新增目录：

- `extension/sched_ext_lab/`

约束：

- 仅在支持 sched_ext 的内核中运行。
- 不支持 sched_ext 不影响 P0/P1。
- 报告必须说明内核版本和 sched_ext ABI 风险。

## 9. PLANv2 一致性检查表

| PRDv2 / TECHv2 要求                    | PLANv2 对应任务     | 状态     |
| ------------------------------------ | --------------- | ------ |
| 新增 `extension/tracebench/tracebench` | Task 1、2        | 已覆盖    |
| `tracebench --help`                  | Task 2、14       | 已覆盖    |
| `tracebench run`                     | Task 6、7、8、9、10 | 已覆盖    |
| `tracebench report`                  | Task 11         | 已覆盖    |
| `tracebench cleanup`                 | Task 12         | 已覆盖    |
| CPU workload                         | Task 6          | 已覆盖    |
| memory workload                      | Task 7          | 已覆盖    |
| I/O workload                         | Task 8          | 已覆盖    |
| cgroup v2 创建、加入、采样、清理                | Task 5、6、9、12   | 已覆盖    |
| 默认 root 权限要求                         | Task 5、6、12、14  | 已覆盖    |
| `--no-cgroup` 低权限演示                  | Task 3、9、14     | 已覆盖    |
| PSI 采样                               | Task 4、9        | 已覆盖    |
| oslab_monitor 对照采样                   | Task 10         | 已覆盖    |
| `samples.csv` 稳定表头                   | 第 6 章、Task 9、14 | 已覆盖    |
| `command.txt`                        | Task 3          | 已覆盖    |
| `environment.txt`                    | Task 3          | 已覆盖    |
| `summary.txt`                        | Task 11         | 已覆盖    |
| Markdown 报告                          | Task 11         | 已覆盖    |
| Bash 集成测试                            | Task 14         | 已覆盖    |
| demo scripts                         | Task 13         | 已覆盖    |
| `.gitignore` 追加规则                    | Task 1          | 已覆盖    |
| README/FEATURES/DOC_AUDIT 同步         | Task 15         | 已覆盖    |
| `TRACEBENCH_REPORT_TEMPLATE.md`      | Task 15         | 已覆盖    |
| P1 tracefs/bpftrace 可降级              | 第 8 章           | 已覆盖为预留 |
| P2 sched_ext 挑战项                     | 第 8 章           | 已覆盖为预留 |
| 不修改基础四模块                             | 第 1、4 章         | 已覆盖    |
| 不自动加载 oslab_monitor                  | Task 10、14      | 已覆盖    |

## 10. 执行说明

执行本计划时应遵守：

- 每个任务完成后运行该任务列出的测试命令。
- 测试通过后再进入下一个任务。
- 代码实现必须在 Ubuntu VM 中验证，尤其是 cgroup v2、PSI 和 root 权限相关功能。
- 当前 Windows 工作区不能作为 P0 完整验收环境。
- `tests/run_all.sh` 不纳入 TraceBench，因为它只运行基础四模块。
- TraceBench P0 验收命令为：

```bash
cd extension/tracebench
sudo bash tests/test_tracebench.sh
```

- 如果实现中改变 CLI、目录结构、CSV 字段、权限语义、输出文件名或 cleanup 范围，必须同步更新 `docs/PRDv2.md`、`docs/TECHv2.md`、`docs/PLANv2.md`、`docs/FEATURES.md` 和 `README.md`。

## 11. 自检清单

- [ ] `docs/PLANv2.md` 存在。
- [ ] `extension/tracebench/Makefile` 存在。
- [ ] `extension/tracebench/include/tracebench.h` 存在。
- [ ] `extension/tracebench/src/main.c` 存在。
- [ ] `extension/tracebench/src/args.c` 存在。
- [ ] `extension/tracebench/src/cgroup.c` 存在。
- [ ] `extension/tracebench/src/workload.c` 存在。
- [ ] `extension/tracebench/src/sampler.c` 存在。
- [ ] `extension/tracebench/src/report.c` 存在。
- [ ] `extension/tracebench/src/util.c` 存在。
- [ ] `extension/tracebench/tests/test_tracebench.sh` 存在。
- [ ] 所有 C/H/SH 文件有中文文件头注释。
- [ ] `make` 能生成 `tracebench`。
- [ ] `tracebench --help` 输出 `run`、`report`、`cleanup`。
- [ ] 非法参数输出 `error:`。
- [ ] 默认非 root `run` 输出 `error:`。
- [ ] 默认非 root `cleanup` 输出 `error:`。
- [ ] CPU profile 可运行并自动结束。
- [ ] memory profile 可运行并自动结束。
- [ ] io profile 可运行并自动结束。
- [ ] workload 子进程加入 cgroup 后才开始施压。
- [ ] `samples.csv` 表头与 TECHv2 固定字段一致。
- [ ] `duration=3 sample-interval=1` 至少产生 3 行采样。
- [ ] `samples.csv` 每行字段数量一致。
- [ ] 默认未加载 oslab_monitor 时 run 不失败。
- [ ] `--with-oslab-monitor` 未加载模块时返回非 `0`。
- [ ] `--no-cgroup` 可低权限运行，cgroup 字段为 `NA`。
- [ ] `summary.txt` 在 run 结束后生成。
- [ ] `tracebench report` 能生成 Markdown。
- [ ] Markdown 包含 `PSI`、`cgroup`、`oslab_monitor` 和 `samples.csv`。
- [ ] cleanup 不删除 `samples.csv`、`summary.txt` 或 Markdown 报告。
- [ ] `sudo bash tests/test_tracebench.sh` 在 Ubuntu VM 中通过。
- [ ] README、FEATURES、DOC_AUDIT 与 v2 实现一致。
- [ ] P1/P2 不影响 P0 测试。

## 12. 执行交接

计划已保存到 `docs/PLANv2.md`。后续执行时可选择以下方式之一：

1. 子任务执行方式：按 Task 1 到 Task 15 顺序逐项实现，每个任务完成后运行对应测试并审查结果。
2. 会话内执行方式：在同一会话中按任务顺序实现，并在关键任务后提交检查点。

默认推荐使用子任务执行方式，因为 TraceBench P0 涉及 cgroup v2、PSI、root 权限和 Ubuntu VM 验证，分任务执行更容易定位失败点。


