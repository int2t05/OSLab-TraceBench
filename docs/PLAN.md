# OS-Design Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 按 `docs/PRD.md`、`docs/TECH.md`、`docs/FEATURES.md` 和 `README.md` 实现操作系统课程设计项目，包括基础四个独立 CLI 模块、Linux `/proc` 扩展模块、用户态工具、测试脚本和报告材料。

**Architecture:** 项目分为基础模拟层和 Linux 系统观测层。基础模拟层包含 `scheduler`、`memory`、`sync`、`filesystem` 四个互相独立的 C 命令行程序；系统观测层包含单文件 Linux 内核模块 `oslab_monitor.c`、用户态工具 `oslabctl.c`、脚本和 Bash 测试。

**Tech Stack:** C、Makefile、POSIX pthread、Bash、Linux Kernel Module、`seq_file`、`/proc`、Ubuntu 22.04/24.04 VM。

---

## 1. 计划边界

本文档只说明要编写的代码文件、测试文件、样例输入、测试命令、预期结果和完成条件，不包含实际代码实现。

实现必须遵守：

- 不新增 PRD 未要求的功能。
- 基础四模块互相独立，不抽取跨模块公共库。
- 每个基础模块采用 `Makefile + include/ + src/ + tests/`。
- 每个基础模块都有 `tests/run_tests.sh`。
- 根目录创建 `tests/run_all.sh`。
- 测试脚本统一使用 Bash。
- 所有 CLI 正常返回 `0`，错误场景返回非 `0` 并输出 `error:`。
- 扩展部分必须在可加载内核模块的 Ubuntu VM 中验证。

## 2. 实现顺序依赖

1. 创建基础目录、扩展目录、测试目录、`.gitignore`。
2. 实现基础模块 `scheduler`。
3. 实现基础模块 `memory`。
4. 实现基础模块 `filesystem`。
5. 实现基础模块 `sync`。
6. 实现扩展模块 kernel `overview`。
7. 实现扩展模块 kernel `tasks` 和 `pid`。
8. 实现用户态工具 `oslabctl`。
9. 实现扩展脚本和扩展测试。
10. 创建根测试脚本和报告提纲。
11. 同步 README、FEATURES、DOC_AUDIT。

## 3. 文件总览

### 3.1 根目录

- Create: `.gitignore`
- Create: `tests/run_all.sh`
- Create: `docs/REPORT_OUTLINE.md`
- Create: `docs/DOC_AUDIT.md`
- Modify: `README.md`
- Modify: `docs/FEATURES.md`

### 3.2 调度模块

- Create: `basic/scheduler/Makefile`
- Create: `basic/scheduler/include/scheduler.h`
- Create: `basic/scheduler/src/main.c`
- Create: `basic/scheduler/src/parser.c`
- Create: `basic/scheduler/src/scheduler.c`
- Create: `basic/scheduler/src/output.c`
- Create: `basic/scheduler/tests/sample.txt`
- Create: `basic/scheduler/tests/idle_case.txt`
- Create: `basic/scheduler/tests/invalid.txt`
- Create: `basic/scheduler/tests/run_tests.sh`

### 3.3 内存模块

- Create: `basic/memory/Makefile`
- Create: `basic/memory/include/memory.h`
- Create: `basic/memory/src/main.c`
- Create: `basic/memory/src/parser.c`
- Create: `basic/memory/src/partition.c`
- Create: `basic/memory/src/paging.c`
- Create: `basic/memory/src/output.c`
- Create: `basic/memory/tests/partition.txt`
- Create: `basic/memory/tests/pages.txt`
- Create: `basic/memory/tests/invalid.txt`
- Create: `basic/memory/tests/run_tests.sh`

### 3.4 文件系统模块

- Create: `basic/filesystem/Makefile`
- Create: `basic/filesystem/include/filesystem.h`
- Create: `basic/filesystem/src/main.c`
- Create: `basic/filesystem/src/parser.c`
- Create: `basic/filesystem/src/filesystem.c`
- Create: `basic/filesystem/src/output.c`
- Create: `basic/filesystem/tests/fs_commands.txt`
- Create: `basic/filesystem/tests/invalid.txt`
- Create: `basic/filesystem/tests/run_tests.sh`

### 3.5 同步模块

- Create: `basic/sync/Makefile`
- Create: `basic/sync/include/sync.h`
- Create: `basic/sync/src/main.c`
- Create: `basic/sync/src/parser.c`
- Create: `basic/sync/src/producer_consumer.c`
- Create: `basic/sync/src/readers_writers.c`
- Create: `basic/sync/src/dining_philosophers.c`
- Create: `basic/sync/src/output.c`
- Create: `basic/sync/tests/run_tests.sh`

### 3.6 扩展模块

- Create: `extension/oslab_monitor/kernel/oslab_monitor.c`
- Create: `extension/oslab_monitor/kernel/Makefile`
- Create: `extension/oslab_monitor/user/oslabctl.c`
- Create: `extension/oslab_monitor/user/Makefile`
- Create: `extension/oslab_monitor/scripts/load.sh`
- Create: `extension/oslab_monitor/scripts/unload.sh`
- Create: `extension/oslab_monitor/scripts/demo.sh`
- Create: `extension/oslab_monitor/tests/test_oslab_monitor.sh`

## 4. 任务列表

### Task 1: 项目骨架和全局忽略规则

**Files:**

- Create: `.gitignore`
- Create directories:
  - `basic/scheduler/include`
  - `basic/scheduler/src`
  - `basic/scheduler/tests`
  - `basic/memory/include`
  - `basic/memory/src`
  - `basic/memory/tests`
  - `basic/sync/include`
  - `basic/sync/src`
  - `basic/sync/tests`
  - `basic/filesystem/include`
  - `basic/filesystem/src`
  - `basic/filesystem/tests`
  - `extension/oslab_monitor/kernel`
  - `extension/oslab_monitor/user`
  - `extension/oslab_monitor/scripts`
  - `extension/oslab_monitor/tests`
  - `tests`

- [ ] **Step 1: 创建目录结构**

Run:

```bash
mkdir -p basic/scheduler/include basic/scheduler/src basic/scheduler/tests
mkdir -p basic/memory/include basic/memory/src basic/memory/tests
mkdir -p basic/sync/include basic/sync/src basic/sync/tests
mkdir -p basic/filesystem/include basic/filesystem/src basic/filesystem/tests
mkdir -p extension/oslab_monitor/kernel extension/oslab_monitor/user
mkdir -p extension/oslab_monitor/scripts extension/oslab_monitor/tests
mkdir -p tests
```

Expected:

- 所有目标目录存在。

- [ ] **Step 2: 创建 `.gitignore`**

`.gitignore` 必须覆盖：

```text
*.o
*.out
*.exe
*.ko
*.mod
*.mod.c
*.cmd
*.symvers
Module.symvers
modules.order
.tmp_versions/
*.log
*.tmp
core
basic/*/scheduler
basic/*/memory
basic/*/sync
basic/*/filesystem
basic/scheduler/scheduler
basic/memory/memory
basic/sync/sync
basic/filesystem/filesystem
extension/oslab_monitor/user/oslabctl
```

Expected:

- 编译产物、内核模块中间文件、日志和临时文件不会进入版本控制。

- [ ] **Step 3: 验证目录结构**

Run:

```bash
test -d basic/scheduler/src
test -d basic/memory/src
test -d basic/sync/src
test -d basic/filesystem/src
test -d extension/oslab_monitor/kernel
test -d tests
```

Expected:

- 所有命令返回 `0`。

- [ ] **Step 4: 建议提交**

Suggested commit:

```bash
git add .gitignore basic extension tests
git commit -m "chore: create project implementation skeleton"
```

**完成条件：**

- 所有目标目录存在。
- `.gitignore` 存在并覆盖 C、内核模块和 CLI 产物。

### Task 2: 调度模块 `scheduler`

**Files:**

- Create: `basic/scheduler/Makefile`
- Create: `basic/scheduler/include/scheduler.h`
- Create: `basic/scheduler/src/main.c`
- Create: `basic/scheduler/src/parser.c`
- Create: `basic/scheduler/src/scheduler.c`
- Create: `basic/scheduler/src/output.c`
- Create: `basic/scheduler/tests/sample.txt`
- Create: `basic/scheduler/tests/idle_case.txt`
- Create: `basic/scheduler/tests/invalid.txt`
- Create: `basic/scheduler/tests/run_tests.sh`

- [ ] **Step 1: 编写测试样例文件**

`basic/scheduler/tests/sample.txt` 内容：

```text
process_count = 4
time_quantum = 2
P1 arrival=0 burst=5 priority=2
P2 arrival=1 burst=3 priority=1
P3 arrival=2 burst=8 priority=4
P4 arrival=3 burst=6 priority=3
```

`basic/scheduler/tests/idle_case.txt` 内容：

```text
process_count = 2
time_quantum = 2
P1 arrival=3 burst=2 priority=1
P2 arrival=7 burst=1 priority=2
```

`basic/scheduler/tests/invalid.txt` 内容：

```text
process_count = 2
time_quantum = 0
P1 arrival=0 burst=5 priority=1
P2 arrival=1 burst=-3 priority=2
```

Expected:

- 固定样例可用于 oracle 校验。
- idle 样例可验证 `IDLE` 时间段。
- invalid 样例可验证 `error:` 和非 `0` 退出码。

- [ ] **Step 2: 编写头文件职责**

`basic/scheduler/include/scheduler.h` 必须声明：

- `Process` 结构。
- `Segment` 结构。
- `Timeline` 结构。
- 输入解析函数。
- FCFS、SJF、RR、Priority 运行函数。
- 输出函数。
- 资源释放函数。

Expected:

- 所有 `.c` 文件通过同一个头文件共享类型。
- `NAME_LEN` 固定为 `32`。

- [ ] **Step 3: 编写 `main.c` 职责**

`main.c` 必须负责：

- 解析 `--algorithm fcfs|sjf|rr|priority`。
- 支持 `--help`。
- 从 stdin 读取输入。
- 调用对应算法。
- 调用输出函数。
- 出错时输出 `error:` 并返回非 `0`。

Expected:

- 不在 `main.c` 中实现调度算法。

- [ ] **Step 4: 编写 `parser.c` 职责**

`parser.c` 必须负责：

- 解析 `process_count = N`。
- 解析 `time_quantum = Q`。
- 解析进程行 `NAME arrival=A burst=B priority=P`。
- 校验 `N > 0`、`Q > 0`、`arrival >= 0`、`burst > 0`。
- 校验进程名长度不超过 31。
- 校验实际进程行数量等于 `process_count`。

Expected:

- 解析失败统一返回错误，由 `main.c` 输出 `error:`。

- [ ] **Step 5: 编写 `scheduler.c` 职责**

`scheduler.c` 必须实现：

- FCFS。
- 非抢占式 SJF。
- RR。
- 非抢占式优先级调度。
- `Timeline` 追加段。
- CPU 空闲时追加 `IDLE[start,end]`。
- 统计 `start_time`、`finish_time`、`waiting_time`、`turnaround_time`、`weighted_turnaround_time`。

Expected:

- 平局规则统一按 PRD：到达时间，再输入顺序。
- 优先级数值越小越高。
- RR 使用普通 ready queue。

- [ ] **Step 6: 编写 `output.c` 职责**

`output.c` 必须输出：

- `algorithm: <name>`
- `timeline: ...`
- 统计表字段：`name arrival burst priority start finish waiting turnaround weighted_turnaround`
- `average_waiting`
- `average_turnaround`
- `average_weighted_turnaround`

Expected:

- 平均值和带权周转时间保留两位小数。

- [ ] **Step 7: 编写 Makefile**

Makefile 必须支持：

```bash
make
make clean
```

Expected:

- `make` 生成 `basic/scheduler/scheduler`。
- 编译包含 `src/*.c` 和 `include/`。

- [ ] **Step 8: 编写 `tests/run_tests.sh`**

测试脚本必须检查：

- `make` 成功。
- `./scheduler --algorithm fcfs < tests/sample.txt` 输出 `P1[0,5] P2[5,8] P3[8,16] P4[16,22]`。
- FCFS 输出 `average_waiting` 包含 `5.75`。
- FCFS 输出 `average_turnaround` 包含 `11.25`。
- SJF 输出 `P1[0,5] P2[5,8] P4[8,14] P3[14,22]`。
- Priority 输出 `P1[0,5] P2[5,8] P4[8,14] P3[14,22]`。
- RR 输出 PRD 固定序列。
- idle 样例输出包含 `IDLE`。
- invalid 样例返回非 `0` 且输出包含 `error:`。

Expected:

- 所有检查通过时脚本返回 `0`。

- [ ] **Step 9: 运行调度模块测试**

Run:

```bash
cd basic/scheduler
bash tests/run_tests.sh
```

Expected:

- 输出测试通过信息。
- 退出码为 `0`。

- [ ] **Step 10: 建议提交**

Suggested commit:

```bash
git add basic/scheduler
git commit -m "feat: implement scheduler module plan target"
```

**完成条件：**

- `scheduler` 可编译。
- FCFS/SJF/RR/Priority 均可运行。
- 固定 oracle 校验通过。
- 错误输入输出 `error:` 并返回非 `0`。

### Task 3: 内存模块 `memory`

**Files:**

- Create: `basic/memory/Makefile`
- Create: `basic/memory/include/memory.h`
- Create: `basic/memory/src/main.c`
- Create: `basic/memory/src/parser.c`
- Create: `basic/memory/src/partition.c`
- Create: `basic/memory/src/paging.c`
- Create: `basic/memory/src/output.c`
- Create: `basic/memory/tests/partition.txt`
- Create: `basic/memory/tests/pages.txt`
- Create: `basic/memory/tests/invalid.txt`
- Create: `basic/memory/tests/run_tests.sh`

- [ ] **Step 1: 编写测试样例文件**

`basic/memory/tests/partition.txt` 内容：

```text
memory_size = 640
alloc P1 130
alloc P2 60
alloc P3 100
free P2
alloc P4 50
```

`basic/memory/tests/pages.txt` 内容：

```text
frame_count = 3
reference_string = 7 0 1 2 0 3 0 4 2 3 0 3 2
```

`basic/memory/tests/invalid.txt` 内容：

```text
frame_count = 0
reference_string =
```

Expected:

- paging 样例用于 FIFO/LRU oracle。
- partition 样例用于 FF/BF、分裂、回收、合并。
- invalid 样例用于错误处理。

- [ ] **Step 2: 编写头文件职责**

`basic/memory/include/memory.h` 必须声明：

- 动态分区结构 `Partition`。
- 分区操作结构 `PartitionOp`。
- 页面置换状态 `PagingState`。
- parser 函数。
- FF/BF 分区函数。
- FIFO/LRU 页面置换函数。
- 输出函数。
- 资源释放函数。

Expected:

- `NAME_LEN` 固定为 `32`。
- 空页框用 `-1`。

- [ ] **Step 3: 编写 `main.c` 职责**

`main.c` 必须负责：

- 解析 `--mode partition|paging`。
- 解析 `--algorithm ff|bf|fifo|lru`。
- 支持 `--help`。
- 根据 mode 分发到 partition 或 paging。
- 错误场景输出 `error:` 并返回非 `0`。

Expected:

- 不在 `main.c` 中实现 FF/BF/FIFO/LRU。

- [ ] **Step 4: 编写 `parser.c` 职责**

`parser.c` 必须负责：

- partition 输入：解析 `memory_size = SIZE`、`alloc NAME SIZE`、`free NAME`。
- paging 输入：解析 `frame_count = N`、`reference_string = ...`。
- 校验所有数值合法。
- 校验 `NAME` 长度。

Expected:

- 空访问序列、负页号、非法内存大小都会产生错误。

- [ ] **Step 5: 编写 `partition.c` 职责**

`partition.c` 必须实现：

- 初始化一个 `[0, memory_size)` 空闲分区。
- FF 选择第一个足够大的空闲分区。
- BF 选择剩余空间最小的空闲分区。
- 分配后拆分分区。
- 回收后合并相邻空闲分区。
- 检查重复分配和回收不存在进程。

Expected:

- 空闲表和已分配表按起始地址升序。

- [ ] **Step 6: 编写 `paging.c` 职责**

`paging.c` 必须实现：

- FIFO。
- LRU。
- 页框数组。
- `loaded_at` 和 `last_used_at`。
- 缺页次数。
- 缺页率。
- 命中时 `evicted = -`。

Expected:

- FIFO 固定样例缺页次数为 `10`。
- LRU 固定样例缺页次数为 `9`。

- [ ] **Step 7: 编写 `output.c` 职责**

`output.c` 必须输出：

- partition：操作编号、操作内容、成功/失败、已分配分区表、空闲分区表。
- paging：访问步号、当前页、页框状态、是否缺页、淘汰页、最终缺页次数、缺页率。

Expected:

- FIFO 缺页率输出 `76.92%`。
- LRU 缺页率输出 `69.23%`。

- [ ] **Step 8: 编写 Makefile**

Makefile 必须支持：

```bash
make
make clean
```

Expected:

- `make` 生成 `basic/memory/memory`。

- [ ] **Step 9: 编写 `tests/run_tests.sh`**

测试脚本必须检查：

- `make` 成功。
- FF 和 BF 可运行并输出分区表。
- FIFO 输出 `page_faults` 为 `10`。
- FIFO 输出 `fault_rate` 为 `76.92%`。
- LRU 输出 `page_faults` 为 `9`。
- LRU 输出 `fault_rate` 为 `69.23%`。
- invalid 样例返回非 `0` 且输出包含 `error:`。

Expected:

- 所有检查通过时脚本返回 `0`。

- [ ] **Step 10: 运行内存模块测试**

Run:

```bash
cd basic/memory
bash tests/run_tests.sh
```

Expected:

- 输出测试通过信息。
- 退出码为 `0`。

- [ ] **Step 11: 建议提交**

Suggested commit:

```bash
git add basic/memory
git commit -m "feat: implement memory module plan target"
```

**完成条件：**

- `memory` 可编译。
- FF/BF/FIFO/LRU 均可运行。
- 固定页面置换 oracle 校验通过。
- 错误输入输出 `error:` 并返回非 `0`。

### Task 4: 文件系统模块 `filesystem`

**Files:**

- Create: `basic/filesystem/Makefile`
- Create: `basic/filesystem/include/filesystem.h`
- Create: `basic/filesystem/src/main.c`
- Create: `basic/filesystem/src/parser.c`
- Create: `basic/filesystem/src/filesystem.c`
- Create: `basic/filesystem/src/output.c`
- Create: `basic/filesystem/tests/fs_commands.txt`
- Create: `basic/filesystem/tests/invalid.txt`
- Create: `basic/filesystem/tests/run_tests.sh`

- [ ] **Step 1: 编写测试样例文件**

`basic/filesystem/tests/fs_commands.txt` 内容：

```text
mkfs 1024 64
mkdir /docs
mkdir /docs/os
create /docs/os/a.txt
write /docs/os/a.txt hello_os
read /docs/os/a.txt
ls /docs/os
delete /docs/os/a.txt
ls /docs/os
stat
```

`basic/filesystem/tests/invalid.txt` 内容：

```text
mkfs 128 64
read /missing.txt
create /docs/a.txt
write /docs/a.txt hello world
```

Expected:

- 正常样例覆盖多级目录、创建、覆盖写、读取、列表、删除、统计。
- 错误样例覆盖不存在文件、父目录不存在、含空格内容。

- [ ] **Step 2: 编写头文件职责**

`basic/filesystem/include/filesystem.h` 必须声明：

- `NodeType`。
- `FsNode`。
- `FileSystem`。
- 命令解析结构。
- `mkfs`、`mkdir`、`create`、`write`、`read`、`ls`、`delete`、`stat` 对应函数。
- 路径解析函数。
- 块分配和释放函数。
- 资源释放函数。

Expected:

- `FS_NAME_LEN = 32`。
- `FS_PATH_LEN = 256`。

- [ ] **Step 3: 编写 `main.c` 职责**

`main.c` 必须负责：

- 支持 `--help`。
- 从 stdin 逐行读取命令。
- 调用 parser 解析命令。
- 调用 filesystem 操作。
- 对每条命令输出结果。
- 错误命令输出 `error:`。

Expected:

- 正常命令继续执行。
- 单条非法命令输出 `error:` 后继续处理后续命令。
- 只要本次输入中出现过命令错误，程序最终返回非 `0`。

- [ ] **Step 4: 编写 `parser.c` 职责**

`parser.c` 必须负责：

- 解析命令名。
- 解析 `mkfs DISK_SIZE BLOCK_SIZE`。
- 解析 `mkdir PATH`。
- 解析 `create PATH`。
- 解析 `write PATH CONTENT`。
- 解析 `read PATH`。
- 解析 `ls PATH`。
- 解析 `delete PATH`。
- 解析 `stat`。
- 拒绝含空格的 `CONTENT`。
- 校验参数数量。

Expected:

- `write /docs/a.txt hello world` 输出 `error:`。

- [ ] **Step 5: 编写 `filesystem.c` 职责**

`filesystem.c` 必须实现：

- 内存型虚拟磁盘。
- 根目录 `/`。
- 多级目录树。
- 块位图。
- 文件节点块列表。
- 覆盖写。
- 删除文件释放块。
- 写入失败时保持旧内容不变。
- 路径合法性校验。

Expected:

- 支持 `/docs/os/a.txt`。
- 删除后 `ls /docs/os` 不再包含 `a.txt`。

- [ ] **Step 6: 编写 `output.c` 职责**

`output.c` 必须输出：

- 命令执行结果。
- `read` 文件内容。
- `ls` 目录条目。
- `stat` 的总块数、已用块数、空闲块数。
- 文件大小和占用块信息。

Expected:

- `stat` 输出包含总块数、已用块数和空闲块数或等价英文字段。

- [ ] **Step 7: 编写 Makefile**

Makefile 必须支持：

```bash
make
make clean
```

Expected:

- `make` 生成 `basic/filesystem/filesystem`。

- [ ] **Step 8: 编写 `tests/run_tests.sh`**

测试脚本必须检查：

- `make` 成功。
- 正常样例输出包含 `hello_os`。
- 删除前 `ls` 输出包含 `a.txt`。
- 删除后第二次 `ls` 不包含 `a.txt`。
- `stat` 输出包含块统计字段。
- invalid 样例输出包含 `error:`。

Expected:

- 所有检查通过时脚本返回 `0`。

- [ ] **Step 9: 运行文件系统模块测试**

Run:

```bash
cd basic/filesystem
bash tests/run_tests.sh
```

Expected:

- 输出测试通过信息。
- 退出码为 `0`。

- [ ] **Step 10: 建议提交**

Suggested commit:

```bash
git add basic/filesystem
git commit -m "feat: implement filesystem module plan target"
```

**完成条件：**

- `filesystem` 可编译。
- 多级目录、文件创建、覆盖写、读取、删除、统计可运行。
- 错误输入输出 `error:`。

### Task 5: 同步模块 `sync`

**Files:**

- Create: `basic/sync/Makefile`
- Create: `basic/sync/include/sync.h`
- Create: `basic/sync/src/main.c`
- Create: `basic/sync/src/parser.c`
- Create: `basic/sync/src/producer_consumer.c`
- Create: `basic/sync/src/readers_writers.c`
- Create: `basic/sync/src/dining_philosophers.c`
- Create: `basic/sync/src/output.c`
- Create: `basic/sync/tests/run_tests.sh`

- [ ] **Step 1: 编写头文件职责**

`basic/sync/include/sync.h` 必须声明：

- 生产者-消费者状态结构。
- 读者-写者状态结构。
- 哲学家进餐状态结构。
- 参数配置结构。
- 三个问题的运行函数。
- 输出辅助函数。

Expected:

- 哲学家数量固定为 `5`。

- [ ] **Step 2: 编写 `main.c` 职责**

`main.c` 必须负责：

- 解析 `--problem producer_consumer|readers_writers|dining_philosophers`。
- 解析 `--producers`、`--consumers`、`--buffer-size`、`--readers`、`--writers`、`--count`。
- 提供默认参数。
- 支持 `--help`。
- 分发到对应问题。
- 参数非法时输出 `error:` 并返回非 `0`。

Expected:

- 默认参数和 PRD 保持一致。

- [ ] **Step 3: 编写 `producer_consumer.c` 职责**

`producer_consumer.c` 必须实现：

- `pthread_mutex_t`。
- `pthread_cond_t not_full`。
- `pthread_cond_t not_empty`。
- 环形缓冲区。
- 每个生产者生产 `count` 次。
- 消费者整体消费 `producers * count` 后退出。
- 汇总字段 `produced_total`、`consumed_total`、`buffer_final_size`。

Expected:

- 缓冲区满时生产者等待。
- 缓冲区空时消费者等待。
- 默认参数下自动结束。

- [ ] **Step 4: 编写 `readers_writers.c` 职责**

`readers_writers.c` 必须实现：

- `pthread_mutex_t`。
- `pthread_cond_t can_read`。
- `pthread_cond_t can_write`。
- 写者优先。
- 读者读时允许多个读者并发。
- 写者写时无其他读者或写者。
- 汇总字段 `read_total`、`write_total`、`final_shared_value`。

Expected:

- 默认参数下自动结束。

- [ ] **Step 5: 编写 `dining_philosophers.c` 职责**

`dining_philosophers.c` 必须实现：

- 5 个哲学家。
- 5 支筷子。
- 每支筷子一个 mutex。
- 最多 4 个哲学家同时尝试拿筷子。
- 每个哲学家吃 `count` 次。
- 输出每个哲学家的 `eat_count`。
- 输出 `deadlock_detected: no`。

Expected:

- 默认参数下自动结束。

- [ ] **Step 6: 编写 `output.c` 职责**

`output.c` 必须输出：

- 线程编号。
- 当前动作。
- 等待、唤醒、加锁、释放资源事件。
- 共享资源状态。
- 汇总统计字段。

Expected:

- 输出能说明同步机制作用。

- [ ] **Step 7: 编写 Makefile**

Makefile 必须支持：

```bash
make
make clean
```

Expected:

- `make` 生成 `basic/sync/sync`。
- 链接 pthread。

- [ ] **Step 8: 编写 `tests/run_tests.sh`**

测试脚本必须检查：

- `make` 成功。
- `timeout 5s ./sync --problem producer_consumer ...` 成功退出。
- producer-consumer 输出 `produced_total`、`consumed_total`、`buffer_final_size`。
- `timeout 5s ./sync --problem readers_writers ...` 成功退出。
- readers-writers 输出 `read_total`、`write_total`、`final_shared_value`。
- `timeout 5s ./sync --problem dining_philosophers ...` 成功退出。
- dining-philosophers 输出 `deadlock_detected: no`。
- 非法参数输出 `error:` 并返回非 `0`。

Expected:

- 所有检查通过时脚本返回 `0`。

- [ ] **Step 9: 运行同步模块测试**

Run:

```bash
cd basic/sync
bash tests/run_tests.sh
```

Expected:

- 输出测试通过信息。
- 退出码为 `0`。

- [ ] **Step 10: 建议提交**

Suggested commit:

```bash
git add basic/sync
git commit -m "feat: implement sync module plan target"
```

**完成条件：**

- `sync` 可编译。
- 三个同步问题均可运行。
- 默认参数下 5 秒内退出。
- 输出汇总字段。
- 错误参数输出 `error:`。

### Task 6: 扩展内核模块 `overview`

**Files:**

- Create: `extension/oslab_monitor/kernel/oslab_monitor.c`
- Create: `extension/oslab_monitor/kernel/Makefile`

- [ ] **Step 1: 编写内核模块 Makefile**

`extension/oslab_monitor/kernel/Makefile` 必须支持：

```bash
make
make clean
```

Expected:

- `make` 在 Ubuntu VM 中生成 `oslab_monitor.ko`。

- [ ] **Step 2: 编写 `oslab_monitor.c` 模块生命周期职责**

`oslab_monitor.c` 必须实现：

- `module_init(oslab_monitor_init)`。
- `module_exit(oslab_monitor_exit)`。
- 创建 `/proc/oslab_monitor/`。
- 创建 `/proc/oslab_monitor/overview`。
- 卸载时删除 `overview` 和目录。
- 加载和卸载时输出内核日志。

Expected:

- `sudo insmod oslab_monitor.ko` 后目录存在。
- `sudo rmmod oslab_monitor` 后目录被清理。

- [ ] **Step 3: 编写 `overview` 接口职责**

`overview` 必须使用 `seq_file` 输出：

- `module: oslab_monitor`
- `kernel: <kernel_release>`
- `total_tasks: <n>`
- `running_tasks: <n>`
- `sleeping_tasks: <n>`
- `stopped_tasks: <n>`
- `zombie_tasks: <n>`
- `mem_total_kb: <n>`
- `mem_free_kb: <n>`
- `mem_available_kb: <n>`
- `read_time_jiffies: <n>`

Expected:

- 字段名稳定。
- 数值非负。

- [ ] **Step 4: 在 Ubuntu VM 验证 overview**

Run in Ubuntu VM:

```bash
cd extension/oslab_monitor/kernel
make
sudo insmod oslab_monitor.ko
cat /proc/oslab_monitor/overview
sudo rmmod oslab_monitor
```

Expected:

- `cat` 输出包含 `module:`、`kernel:`、`total_tasks:`、`mem_total_kb:`。
- 卸载后 `/proc/oslab_monitor/` 不存在。

- [ ] **Step 5: 建议提交**

Suggested commit:

```bash
git add extension/oslab_monitor/kernel
git commit -m "feat: add oslab monitor overview proc interface"
```

**完成条件：**

- `oslab_monitor.ko` 可编译。
- `overview` 可读。
- 模块可加载和卸载。

### Task 7: 扩展内核模块 `tasks` 和 `pid`

**Files:**

- Modify: `extension/oslab_monitor/kernel/oslab_monitor.c`
- Modify: `extension/oslab_monitor/kernel/Makefile`

- [ ] **Step 1: 扩展 `/proc` 节点创建**

`oslab_monitor.c` 必须新增：

- `/proc/oslab_monitor/tasks`，权限 `0444`。
- `/proc/oslab_monitor/pid`，权限 `0644`。

Expected:

- `ls /proc/oslab_monitor` 输出包含 `overview`、`tasks`、`pid`。

- [ ] **Step 2: 编写 `tasks` 接口职责**

`tasks` 必须使用 `seq_file` 输出表头：

```text
PID     COMM            STATE   POLICY  PRIO  NICE  THREADS  RSS_KB  MIN_FLT  MAJ_FLT
```

必须遍历全部任务并输出：

- PID。
- COMM。
- STATE。
- POLICY。
- PRIO。
- NICE。
- THREADS。
- RSS_KB 或 `N/A`。
- MIN_FLT 或 `N/A`。
- MAJ_FLT 或 `N/A`。

Expected:

- 不固定小样本数量。
- `mm == NULL` 时不访问空指针。
- 使用完 `mm_struct` 后释放。

- [ ] **Step 3: 编写 `pid` 写入职责**

`pid` 写入必须：

- 使用 `copy_from_user()`。
- 限制输入长度。
- 使用整数解析。
- 拒绝小于等于 `0` 的 PID。
- 权限固定为 `0644`。
- 保存当前目标 PID。

Expected:

- 非法写入不会导致模块崩溃。

- [ ] **Step 4: 编写 `pid` 读取职责**

`pid` 读取必须输出：

- `pid:`
- `comm:`
- `state:`
- `ppid:`
- `policy:`
- `prio:`
- `nice:`
- `threads:`
- `rss_kb:`

PID 不存在时输出 `not found` 或等价字段。

Expected:

- `echo 1 | sudo tee /proc/oslab_monitor/pid` 后读取能输出 PID 1 详情。

- [ ] **Step 5: 在 Ubuntu VM 验证 tasks/pid**

Run in Ubuntu VM:

```bash
cd extension/oslab_monitor/kernel
make
sudo insmod oslab_monitor.ko
ls /proc/oslab_monitor
cat /proc/oslab_monitor/tasks | head
echo 1 | sudo tee /proc/oslab_monitor/pid
cat /proc/oslab_monitor/pid
sudo rmmod oslab_monitor
```

Expected:

- `tasks` 输出表头和至少一条进程记录。
- `pid` 输出包含 `pid:` 和 `comm:`。
- 模块卸载后 `/proc/oslab_monitor/` 不存在。

- [ ] **Step 6: 建议提交**

Suggested commit:

```bash
git add extension/oslab_monitor/kernel
git commit -m "feat: add oslab monitor task and pid proc interfaces"
```

**完成条件：**

- `overview`、`tasks`、`pid` 三个节点存在。
- `tasks` 和 `pid` 可读。
- `pid` 可由 root 写入。
- 模块可卸载并清理。

### Task 8: 用户态工具 `oslabctl`

**Files:**

- Create: `extension/oslab_monitor/user/oslabctl.c`
- Create: `extension/oslab_monitor/user/Makefile`

- [ ] **Step 1: 编写用户态 Makefile**

`extension/oslab_monitor/user/Makefile` 必须支持：

```bash
make
make clean
```

Expected:

- `make` 生成 `extension/oslab_monitor/user/oslabctl`。

- [ ] **Step 2: 编写 `oslabctl.c` 命令职责**

`oslabctl.c` 必须支持：

- `./oslabctl --help`
- `./oslabctl overview`
- `./oslabctl tasks`
- `sudo ./oslabctl pid <PID>`

Expected:

- `overview` 原样输出 `/proc/oslab_monitor/overview`。
- `tasks` 原样输出 `/proc/oslab_monitor/tasks`。
- `pid` 写入 `/proc/oslab_monitor/pid` 后原样读取输出。

- [ ] **Step 3: 编写错误处理职责**

`oslabctl.c` 必须处理：

- `/proc/oslab_monitor/` 不存在：提示模块未加载。
- proc 文件打开失败：输出路径和系统错误原因。
- `pid` 参数缺失：输出错误和帮助。
- `pid` 不是正整数：输出错误和帮助。
- 写入 `pid` 权限不足：提示 `sudo ./oslabctl pid <PID>`。

Expected:

- 所有错误返回非 `0`。

- [ ] **Step 4: 在 Ubuntu VM 验证 oslabctl**

Run in Ubuntu VM:

```bash
cd extension/oslab_monitor/kernel
make
sudo insmod oslab_monitor.ko
cd ../user
make
./oslabctl --help
./oslabctl overview
./oslabctl tasks
sudo ./oslabctl pid 1
cd ../kernel
sudo rmmod oslab_monitor
```

Expected:

- `overview` 与直接读取 `/proc/oslab_monitor/overview` 字段一致。
- `tasks` 与直接读取 `/proc/oslab_monitor/tasks` 字段一致。
- `pid 1` 输出包含 `pid:` 和 `comm:`。

- [ ] **Step 5: 建议提交**

Suggested commit:

```bash
git add extension/oslab_monitor/user
git commit -m "feat: add oslabctl proc wrapper"
```

**完成条件：**

- `oslabctl` 可编译。
- `overview`、`tasks`、`pid` 三个命令可运行。
- 模块未加载、参数错误、权限不足都有明确提示。

### Task 9: 扩展脚本和扩展测试

**Files:**

- Create: `extension/oslab_monitor/scripts/load.sh`
- Create: `extension/oslab_monitor/scripts/unload.sh`
- Create: `extension/oslab_monitor/scripts/demo.sh`
- Create: `extension/oslab_monitor/tests/test_oslab_monitor.sh`

- [ ] **Step 1: 编写 `load.sh` 职责**

`load.sh` 必须：

- 进入 `kernel/`。
- 执行 `make`。
- 执行 `sudo insmod oslab_monitor.ko`。
- 检查 `/proc/oslab_monitor/` 存在。
- 输出 `lsmod` 或等价检查结果。

Expected:

- 加载成功时返回 `0`。

- [ ] **Step 2: 编写 `unload.sh` 职责**

`unload.sh` 必须：

- 执行 `sudo rmmod oslab_monitor`。
- 检查 `/proc/oslab_monitor/` 不存在。
- 输出 `dmesg | tail` 供报告使用。

Expected:

- 卸载成功时返回 `0`。

- [ ] **Step 3: 编写 `demo.sh` 职责**

`demo.sh` 必须：

- 输出 `overview`。
- 输出 `tasks | head`。
- 写入 PID 1。
- 读取 PID 1。
- 构建并运行 `oslabctl overview`。
- 构建并运行 `oslabctl tasks`。
- 运行 `sudo oslabctl pid 1`。

Expected:

- 输出可用于课程报告截图或文本留存。

- [ ] **Step 4: 编写 `test_oslab_monitor.sh` 职责**

测试脚本必须：

- 使用 `trap` 在失败或退出时尝试卸载模块。
- 编译 kernel。
- 加载模块。
- 检查 `/proc/oslab_monitor/overview` 包含 `module:`、`kernel:`、`total_tasks:`、`mem_total_kb:`。
- 检查 `/proc/oslab_monitor/tasks` 包含表头。
- 写入 PID 1 并检查读取结果包含 `pid:` 和 `comm:`。
- 编译 user。
- 运行 `oslabctl overview`。
- 运行 `oslabctl tasks`。
- 运行 `sudo oslabctl pid 1`。
- 卸载模块。
- 检查 `/proc/oslab_monitor/` 不存在。

Expected:

- 所有检查通过时脚本返回 `0`。

- [ ] **Step 5: 在 Ubuntu VM 运行扩展测试**

Run in Ubuntu VM:

```bash
cd extension/oslab_monitor
bash tests/test_oslab_monitor.sh
```

Expected:

- 测试通过。
- 失败时 trap 尽量卸载模块。

- [ ] **Step 6: 建议提交**

Suggested commit:

```bash
git add extension/oslab_monitor/scripts extension/oslab_monitor/tests
git commit -m "test: add oslab monitor scripts and integration test"
```

**完成条件：**

- 扩展加载、演示、卸载脚本存在。
- 扩展测试脚本包含 trap 清理。
- Ubuntu VM 中扩展测试通过。

### Task 10: 根测试脚本 `tests/run_all.sh`

**Files:**

- Create: `tests/run_all.sh`

- [ ] **Step 1: 编写根测试脚本职责**

`tests/run_all.sh` 必须调用：

- `basic/scheduler/tests/run_tests.sh`
- `basic/memory/tests/run_tests.sh`
- `basic/filesystem/tests/run_tests.sh`
- `basic/sync/tests/run_tests.sh`

扩展测试不强制纳入根测试，因为它需要 Ubuntu VM、root 权限和可加载内核模块环境。脚本应输出提示说明扩展测试命令：

```bash
cd extension/oslab_monitor
bash tests/test_oslab_monitor.sh
```

Expected:

- 基础模块测试全部通过时返回 `0`。
- 任一基础模块失败时返回非 `0`。

- [ ] **Step 2: 运行根测试**

Run:

```bash
bash tests/run_all.sh
```

Expected:

- scheduler、memory、filesystem、sync 测试全部通过。
- 输出扩展测试需在 Ubuntu VM 中单独运行的说明。

- [ ] **Step 3: 建议提交**

Suggested commit:

```bash
git add tests/run_all.sh
git commit -m "test: add root test runner"
```

**完成条件：**

- 根测试脚本存在。
- 基础四模块测试可一键运行。
- 扩展测试环境限制有明确说明。

### Task 11: 报告提纲和文档同步

**Files:**

- Create: `docs/REPORT_OUTLINE.md`
- Create: `docs/DOC_AUDIT.md`
- Modify: `README.md`
- Modify: `docs/FEATURES.md`

- [ ] **Step 1: 编写 `docs/REPORT_OUTLINE.md` 职责**

`docs/REPORT_OUTLINE.md` 必须包含课程报告章节：

- 项目名称。
- 小组成员及贡献说明。
- 代码 URL 和访问方式。
- 运行环境。
- 项目目录结构。
- 基础四模块设计与实现说明。
- 基础四模块运行截图或输出文本。
- 扩展部分选题背景和实践价值。
- Linux 内核模块设计说明。
- `/proc/oslab_monitor/overview`、`tasks`、`pid` 接口说明。
- `oslabctl` 用户态工具说明。
- 扩展部分运行截图或输出文本。
- 遇到的问题与解决方案。
- 项目总结。
- 参考资料。

Expected:

- 报告提纲覆盖 PRD 第 13 章所有要求。

- [ ] **Step 2: 同步 README 职责**

实现完成后，`README.md` 必须更新：

- 当前仓库状态从“文档设计阶段”改为“实现完成”或“部分实现完成”。
- 补充实际运行结果说明。
- 保留基础和扩展目标命令。
- 若某些验证必须在 Ubuntu VM 中运行，明确说明。

Expected:

- README 不再声称只有文档完成，除非实现尚未完成。

- [ ] **Step 3: 同步 FEATURES 职责**

实现完成后，`docs/FEATURES.md` 必须更新：

- 若实现功能与计划一致，保持功能清单不变。
- 若某项功能未完成，必须明确标注状态，不能隐性遗漏。

Expected:

- FEATURES 与实际代码功能一致。

- [ ] **Step 4: 创建或同步 DOC_AUDIT 职责**

实现完成后，`docs/DOC_AUDIT.md` 必须创建或同步，并记录：

- 新增源码目录。
- 新增测试脚本。
- 文档与代码结构是否一致。
- 尚未验证的环境项。

Expected:

- 文档审计报告反映实现后状态。

- [ ] **Step 5: 建议提交**

Suggested commit:

```bash
git add docs/REPORT_OUTLINE.md README.md docs/FEATURES.md docs/DOC_AUDIT.md
git commit -m "docs: add report outline and sync project documentation"
```

**完成条件：**

- 报告提纲存在。
- README、FEATURES、DOC_AUDIT 与实际实现状态一致。

## 5. 计划一致性检查表

| 来源要求 | PLAN 对应任务 | 状态 |
|---|---|---|
| 基础四个独立 CLI | Task 2、3、4、5 | 已覆盖 |
| 每个基础模块 `Makefile + include + src + tests` | Task 1、2、3、4、5 | 已覆盖 |
| 每个基础模块 `tests/run_tests.sh` | Task 2、3、4、5 | 已覆盖 |
| 根测试脚本 | Task 10 | 已覆盖 |
| 调度 FCFS/SJF/RR/Priority | Task 2 | 已覆盖 |
| 调度固定 oracle | Task 2 | 已覆盖 |
| 内存 FF/BF/FIFO/LRU | Task 3 | 已覆盖 |
| 页面置换固定 oracle | Task 3 | 已覆盖 |
| 文件系统多级目录、块位图、覆盖写 | Task 4 | 已覆盖 |
| 同步 producer_consumer/readers_writers/dining_philosophers | Task 5 | 已覆盖 |
| 同步测试使用 `timeout 5s` | Task 5 | 已覆盖 |
| 内核模块 `overview` | Task 6 | 已覆盖 |
| 内核模块 `tasks` 和 `pid` | Task 7 | 已覆盖 |
| `/proc` 权限 `overview/tasks = 0444`、`pid = 0644` | Task 7 | 已覆盖 |
| `oslabctl` 原样输出 `/proc` 内容 | Task 8 | 已覆盖 |
| 扩展脚本和 trap 清理测试 | Task 9 | 已覆盖 |
| Ubuntu VM 验证说明 | Task 6、7、8、9、10 | 已覆盖 |
| 报告提纲 | Task 11 | 已覆盖 |
| 文档同步 | Task 11 | 已覆盖 |

## 6. 执行说明

执行计划时应遵守：

- 每个任务完成后运行该任务列出的测试命令。
- 测试通过后再进入下一个任务。
- 基础模块可以在 Linux 命令行环境中验证。
- 扩展模块必须在 Ubuntu 22.04 或 Ubuntu 24.04 VM 中验证。
- 当前 Windows 工作区不能作为内核模块加载验收环境。
- 若实现中改变了 CLI、目录结构、输出字段或权限，必须同步更新 `docs/PRD.md`、`docs/TECH.md`、`docs/FEATURES.md` 和 `README.md`。

## 7. 自检清单

- [ ] 无跨基础模块公共库。
- [ ] 所有基础 CLI 支持 `--help`。
- [ ] 所有基础 CLI 错误场景输出 `error:`。
- [ ] 所有基础 CLI 正常场景返回 `0`。
- [ ] 所有基础 CLI 错误场景返回非 `0`。
- [ ] 所有基础模块 `make clean` 可清理产物。
- [ ] 所有基础模块 `tests/run_tests.sh` 可单独运行。
- [ ] `tests/run_all.sh` 可运行基础四模块测试。
- [ ] 扩展模块卸载后 `/proc/oslab_monitor/` 不存在。
- [ ] `oslabctl overview` 和直接读取 `/proc/oslab_monitor/overview` 字段一致。
- [ ] `oslabctl tasks` 和直接读取 `/proc/oslab_monitor/tasks` 字段一致。
- [ ] `sudo oslabctl pid 1` 可输出 PID 1 信息。
- [ ] README 与实际实现状态一致。
- [ ] FEATURES 与实际功能一致。
- [ ] REPORT_OUTLINE 覆盖 PRD 报告要求。

## 8. 执行交接

Plan complete and saved to `docs/PLAN.md`. Two execution options:

1. Subagent-Driven (recommended) - dispatch a fresh subagent per task, review between tasks, fast iteration.
2. Inline Execution - execute tasks in this session using executing-plans, batch execution with checkpoints.
