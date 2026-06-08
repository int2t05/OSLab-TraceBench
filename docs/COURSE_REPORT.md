# OS-Design 操作系统课程设计报告

## 1. 项目基本信息

项目名称：OS-Design 操作系统课程设计

项目类型：基础必做部分 + 自由扩展提升部分

代码仓库：`https://github.com/int2t05/OS-Design`

开发语言：C

构建工具：Makefile

测试方式：Bash 自动化测试脚本

扩展方向：Linux 内核模块与 `/proc` 接口

本项目围绕操作系统课程中的几个核心内容展开。基础部分实现处理机调度、内存管理、进程同步与并发控制、文件系统四个模块。扩展部分实现一个 Linux 内核模块，通过 `/proc/oslab_monitor/` 暴露当前系统的进程和内存信息，并提供用户态命令行工具 `oslabctl` 进行访问。

项目实现时尽量保持每个模块独立，避免把不同实验内容混在一起。这样做的好处是验收和调试都比较清楚：调度模块只负责调度，内存模块只负责内存，文件系统模块只负责虚拟文件系统，同步模块只负责并发控制，扩展模块只负责真实 Linux 系统观测。

## 2. 运行环境

基础部分在普通 Linux 命令行环境下运行，主要依赖如下：

```text
gcc
make
bash
pthread
```

扩展部分需要能加载内核模块的 Ubuntu 虚拟机环境，主要依赖如下：

```text
Ubuntu 22.04 LTS 或 Ubuntu 24.04 LTS
build-essential
linux-headers-$(uname -r)
insmod
rmmod
lsmod
dmesg
```

本次最终验证环境为：

```text
Ubuntu 24.04.2 LTS
Linux ubuntu2404 6.11.0-17-generic
gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
GNU Make 4.3
/lib/modules/6.11.0-17-generic/build present
```

说明：WSL2 默认不适合作为本项目扩展部分的验收环境。原因是 WSL2 内核模块编译和加载受环境限制，常见问题是 `/lib/modules/$(uname -r)/build` 不存在。基础模块可以在 WSL2 中运行，扩展模块最终需要在 Ubuntu VM 中验证。

## 3. 项目目录结构

项目根目录结构如下：

```text
OS-Design/
├── basic/
│   ├── scheduler/       处理机调度模块
│   ├── memory/          内存管理模块
│   ├── sync/            进程同步与并发控制模块
│   └── filesystem/      文件系统模块
├── extension/
│   └── oslab_monitor/   Linux 内核观测扩展模块
├── tests/
│   └── run_all.sh       基础模块一键测试脚本
├── docs/
│   ├── COURSE_REPORT.md 课程设计报告
│   ├── PRD.md           需求文档
│   ├── TECH.md          技术方案
│   ├── FEATURES.md      功能清单
│   ├── PLAN.md          实现计划
│   └── DOC_AUDIT.md     文档一致性审计
└── README.md            仓库运行说明
```

基础四个模块都采用相似结构：

```text
basic/<module>/
├── Makefile
├── include/
├── src/
└── tests/
```

这种结构比较适合课程设计验收。源码、头文件、测试样例和构建脚本都在各自模块目录下，单个模块可以独立编译、运行和清理。

## 4. 总体设计

本项目分为两层：

第一层是基础模拟层。它用用户态 C 程序模拟操作系统中的典型算法和机制，包括调度算法、内存分配、页面置换、线程同步和简易文件系统。基础模拟层不依赖内核模块，也不依赖扩展部分。

第二层是 Linux 系统观测层。它由一个 Linux 内核模块和一个用户态工具组成。内核模块负责读取真实 Linux 内核中的进程和内存信息，并通过 `/proc` 文件输出；用户态工具只负责读取或写入 `/proc` 文件，不重新格式化内核输出。

项目采用四个基础 CLI，而不是统一菜单程序。这样虽然会重复少量参数解析代码，但模块边界更清楚，也更符合课程设计中按模块验收的方式。

## 5. 处理机调度模块

### 5.1 模块目标

处理机调度模块位于 `basic/scheduler/`。它实现四种调度算法：

- FCFS：先来先服务。
- SJF：非抢占式短作业优先。
- RR：时间片轮转。
- Priority：非抢占式优先级调度。

模块入口为：

```bash
./scheduler --algorithm fcfs|sjf|rr|priority < tests/sample.txt
```

输入样例：

```text
process_count = 4
time_quantum = 2
P1 arrival=0 burst=5 priority=2
P2 arrival=1 burst=3 priority=1
P3 arrival=2 burst=8 priority=4
P4 arrival=3 burst=6 priority=3
```

### 5.2 数据结构设计

调度模块的核心结构包括：

- `Process`：保存进程名、到达时间、服务时间、优先级、剩余时间、开始时间、完成时间和输入顺序。
- `Segment`：表示甘特图中的一个运行片段。
- `Timeline`：保存完整执行序列。

`Process` 中保存 `input_order` 是为了处理平局情况。例如两个进程到达时间相同，或者 SJF 中服务时间相同，就按输入顺序选择。这样输出结果稳定，便于测试脚本比对。

### 5.3 算法实现

FCFS 按到达时间升序选择进程，到达时间相同时按输入顺序。若当前时间小于下一个进程到达时间，则在时间线中加入 `IDLE[start,end]`，表示 CPU 空闲。

SJF 是非抢占式实现。每次 CPU 空闲时，从已经到达且未完成的进程中选择服务时间最短的进程。进程一旦开始运行，就一直运行到完成，不会被后续更短的进程抢占。

Priority 也是非抢占式实现。优先级数值越小表示优先级越高。只在 CPU 空闲时选择一次进程，不在运行中途重新选择。

RR 使用普通就绪队列。进程按到达顺序入队，每次取队头运行一个时间片。如果进程未完成，则在本时间片内新到达的进程入队之后，再把该进程放回队尾。这样能保持时间片轮转的队列顺序。

### 5.4 输出内容

模块输出包括：

- 算法名称。
- 甘特图执行序列。
- 每个进程的开始时间、完成时间、等待时间、周转时间、带权周转时间。
- 平均等待时间、平均周转时间、平均带权周转时间。

所有平均值保留两位小数。

### 5.5 测试结果

调度模块测试命令：

```bash
cd basic/scheduler
bash tests/run_tests.sh
```

测试覆盖：

- FCFS 固定执行序列。
- SJF 固定执行序列。
- Priority 固定执行序列。
- RR 固定执行序列。
- CPU 空闲时输出 `IDLE`。
- 非法输入输出 `error:` 并返回非 0。

固定样例中，测试检查了以下关键结果：

```text
FCFS average_waiting: 5.75
FCFS average_turnaround: 11.25
SJF timeline: P1[0,5] P2[5,8] P4[8,14] P3[14,22]
RR timeline: P1[0,2] P2[2,4] P3[4,6] ...
```

## 6. 内存管理模块

### 6.1 模块目标

内存管理模块位于 `basic/memory/`。它包含两部分：

- 动态分区管理：首次适应 FF、最佳适应 BF。
- 页面置换：FIFO、LRU。

运行命令：

```bash
./memory --mode partition --algorithm ff < tests/partition.txt
./memory --mode partition --algorithm bf < tests/partition.txt
./memory --mode paging --algorithm fifo < tests/pages.txt
./memory --mode paging --algorithm lru < tests/pages.txt
```

### 6.2 动态分区设计

动态分区采用链表表示。每个链表节点表示一个内存分区，包含：

- 起始地址。
- 分区大小。
- 是否空闲。
- 所属进程名。
- 下一个分区指针。

初始化时只有一个完整空闲分区。例如 `memory_size = 640` 时，初始分区为：

```text
start = 0
size = 640
free = 1
```

FF 从低地址到高地址扫描，找到第一个足够大的空闲分区。BF 扫描全部空闲分区，选择能容纳申请且剩余空间最小的分区。如果剩余空间相同，就选择起始地址更小的分区。

分配时，如果空闲分区大小大于申请大小，就把原节点改成已分配节点，并在后面插入一个剩余空闲节点。回收时，先把目标分区标记为空闲，再从链表头扫描并合并相邻空闲分区。

### 6.3 页面置换设计

页面置换使用数组保存页框状态。空页框用 `-1` 表示。

FIFO 使用 `loaded_at` 记录页面进入页框的时间，淘汰时选择最早进入的页面。

LRU 使用 `last_used_at` 记录页面最近访问时间。命中时更新最近访问时间；缺页且需要淘汰时，选择最久未访问的页面。

每次访问页面后都会保存一步状态，包括：

- 当前页号。
- 页框内容。
- 是否缺页。
- 被淘汰页面，若没有淘汰则输出 `-`。

### 6.4 测试结果

内存模块测试命令：

```bash
cd basic/memory
bash tests/run_tests.sh
```

测试覆盖：

- FF 和 BF 动态分区可运行。
- 分配、回收、空闲分区表输出。
- FIFO 页面置换缺页次数和缺页率。
- LRU 页面置换缺页次数和缺页率。
- 非法输入输出 `error:` 并返回非 0。

页面置换固定样例：

```text
frame_count = 3
reference_string = 7 0 1 2 0 3 0 4 2 3 0 3 2
```

测试结果：

```text
FIFO page_faults: 10
FIFO fault_rate: 76.92%
LRU page_faults: 9
LRU fault_rate: 69.23%
```

从结果可以看出，在这个访问序列下，LRU 比 FIFO 少一次缺页。原因是 LRU 利用了最近访问历史，更容易保留近期还会再次访问的页面。

## 7. 文件系统模块

### 7.1 模块目标

文件系统模块位于 `basic/filesystem/`。它实现一个运行期内存型虚拟文件系统，支持：

- `mkfs` 初始化虚拟磁盘。
- `mkdir` 创建目录。
- `create` 创建文件。
- `write` 覆盖写文件。
- `read` 读取文件。
- `ls` 列出目录。
- `delete` 删除文件。
- `stat` 查看块使用情况。

运行命令：

```bash
cd basic/filesystem
make
./filesystem < tests/fs_commands.txt
```

### 7.2 数据结构设计

文件系统由三部分组成：

- 目录树：用 `FsNode` 表示目录和文件。
- 块位图：用 `block_used[i]` 表示第 `i` 个块是否已使用。
- 块数据数组：用 `block_data[i]` 保存每个块中的内容片段。

目录节点保存子节点数组，文件节点保存文件大小和块列表。根目录固定为 `/`。

路径要求为绝对路径，例如：

```text
/docs/os/a.txt
```

每一级名称只允许字母、数字、下划线、短横线和点号，路径长度也有上限。这是为了避免输入过长或非法字符导致缓冲区问题。

### 7.3 文件写入设计

写文件采用覆盖写，不支持追加写。写入前先计算新内容需要多少块，并检查空闲块是否足够。

这里有一个重要处理：如果空间不足，不能破坏旧文件内容。因此实现时先检查空闲块数量，并先准备新内容块。只有确认可以成功写入后，才释放旧块并替换为新块。

删除文件时，会从父目录中移除文件节点，并释放其占用的所有块。

### 7.4 测试结果

文件系统模块测试命令：

```bash
cd basic/filesystem
bash tests/run_tests.sh
```

测试样例覆盖：

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

测试检查：

- `read` 输出包含 `hello_os`。
- 删除前 `ls` 能看到 `a.txt`。
- 删除后 `ls` 不再包含 `a.txt`。
- `stat` 输出总块数、已用块数、空闲块数。
- 非法命令输出 `error:`。

## 8. 进程同步与并发控制模块

### 8.1 模块目标

同步模块位于 `basic/sync/`。它使用 POSIX pthread 实现三个经典同步问题：

- 生产者-消费者。
- 读者-写者。
- 哲学家进餐。

运行命令：

```bash
./sync --problem producer_consumer --producers 2 --consumers 2 --buffer-size 4 --count 10
./sync --problem readers_writers --readers 3 --writers 2 --count 5
./sync --problem dining_philosophers --count 3
```

### 8.2 生产者-消费者

生产者-消费者使用：

- `pthread_mutex_t mutex`
- `pthread_cond_t not_full`
- `pthread_cond_t not_empty`

缓冲区是环形队列。生产者在缓冲区满时等待 `not_full`，消费者在缓冲区空时等待 `not_empty`。

退出条件使用全局消费计数 `consumed_total`。消费者整体消费目标为 `producers * count`。这样即使生产者和消费者数量不相等，也不会出现消费者一直等待无法退出的问题。

### 8.3 读者-写者

读者-写者采用写者优先策略。核心状态包括：

- 当前活动读者数。
- 当前活动写者数。
- 等待写者数。
- 共享数据值。

当有写者活动时，读者必须等待。当有写者等待时，新读者也等待。这样可以避免写者长期饥饿。

### 8.4 哲学家进餐

哲学家进餐固定 5 个哲学家和 5 支筷子。每支筷子用一个互斥锁表示。

为避免死锁，程序限制最多 4 个哲学家同时尝试拿筷子。这样至少有一名哲学家不会参与竞争，从而避免所有人都拿到一支筷子并互相等待的情况。

### 8.5 测试结果

同步模块测试命令：

```bash
cd basic/sync
bash tests/run_tests.sh
```

测试使用 `timeout 5s` 检查三个问题都能正常退出，避免死锁或线程无法结束。

测试检查：

- 生产者-消费者输出 `produced_total`、`consumed_total`、`buffer_final_size`。
- 读者-写者输出 `read_total`、`write_total`、`final_shared_value`。
- 哲学家进餐输出 `deadlock_detected: no`。
- 非法参数输出 `error:`。

## 9. Linux 内核扩展模块

### 9.1 模块目标

扩展模块位于 `extension/oslab_monitor/`。它实现 Linux 内核模块 `oslab_monitor.ko`，加载后创建：

```text
/proc/oslab_monitor/overview
/proc/oslab_monitor/tasks
/proc/oslab_monitor/pid
```

并提供用户态工具：

```text
extension/oslab_monitor/user/oslabctl
```

### 9.2 模块生命周期

模块加载时执行：

1. 创建 `/proc/oslab_monitor/` 目录。
2. 创建 `overview`、`tasks`、`pid` 三个节点。
3. 初始化默认目标 PID。
4. 输出加载日志。

模块卸载时执行：

1. 删除 `pid`。
2. 删除 `tasks`。
3. 删除 `overview`。
4. 删除 `/proc/oslab_monitor/` 目录。
5. 输出卸载日志。

清理顺序与创建顺序相反，避免卸载后残留 `/proc` 节点。

### 9.3 `/proc/oslab_monitor/overview`

`overview` 输出系统概览信息，包括：

```text
module:
kernel:
total_tasks:
running_tasks:
sleeping_tasks:
stopped_tasks:
zombie_tasks:
mem_total_kb:
mem_free_kb:
mem_available_kb:
read_time_jiffies:
```

实现中使用 `for_each_process(task)` 遍历进程，统计进程总数和状态分布。内存信息通过 `si_meminfo()` 和 `si_mem_available()` 获取。

### 9.4 `/proc/oslab_monitor/tasks`

`tasks` 输出当前系统进程列表。表头为：

```text
PID     COMM            STATE   POLICY  PRIO  NICE  THREADS  RSS_KB  MIN_FLT  MAJ_FLT
```

该接口遍历全部进程，不设置固定数量上限。长文本输出使用 `seq_file`，避免手写固定缓冲区导致输出被截断。

RSS 读取时使用 `get_task_mm()` 获取 `mm_struct`。如果任务是内核线程，`mm == NULL`，则 RSS 输出为 0。使用完 `mm_struct` 后调用 `mmput(mm)` 释放引用。

主缺页和次缺页字段在当前实现中保留输出位置，输出为 `N/A`。这样可以保持字段格式稳定，同时避免访问不同内核版本中不稳定或受限的字段。

### 9.5 `/proc/oslab_monitor/pid`

`pid` 节点用于查询指定 PID 信息。写入一个 PID 后，再读取该文件即可看到对应进程详情。

示例：

```bash
echo 1 | sudo tee /proc/oslab_monitor/pid
cat /proc/oslab_monitor/pid
```

输出字段包括：

```text
pid:
comm:
state:
ppid:
policy:
prio:
nice:
threads:
rss_kb:
```

写入 PID 时使用 `copy_from_user()` 复制用户输入，并限制输入长度为 31 字节以内。解析使用 `kstrtoint()`，小于等于 0 的 PID 会被拒绝。目标 PID 使用互斥锁保护，避免并发读写时出现不一致。

### 9.6 `/proc` 权限设计

权限设置如下：

```text
overview = 0444
tasks    = 0444
pid      = 0644
```

`overview` 和 `tasks` 只读，普通用户可以查看。`pid` 允许读取，但写入目标 PID 属于控制行为，因此默认需要 root 权限。

### 9.7 oslabctl 用户态工具

`oslabctl` 是用户态命令行工具，支持：

```bash
./oslabctl --help
./oslabctl overview
./oslabctl tasks
sudo ./oslabctl pid 1
```

该工具只封装 `/proc` 文件访问，不重新格式化输出。这样 `oslabctl overview` 与 `cat /proc/oslab_monitor/overview` 的输出保持一致，测试和验收都更直接。

如果内核模块未加载、参数错误或权限不足，工具会输出明确错误并返回非 0。

## 10. 测试与验证

### 10.1 基础模块测试

基础四模块一键测试命令：

```bash
bash tests/run_all.sh
```

Ubuntu VM 中测试结果：

```text
== running scheduler tests ==
scheduler tests passed
== running memory tests ==
memory tests passed
== running filesystem tests ==
filesystem tests passed
== running sync tests ==
sync tests passed
basic module tests passed
```

### 10.2 扩展模块测试

扩展模块集成测试命令：

```bash
cd extension/oslab_monitor
bash tests/test_oslab_monitor.sh
```

Ubuntu VM 中测试结果：

```text
make -C /lib/modules/6.11.0-17-generic/build M=... modules
CC [M] oslab_monitor.o
LD [M] oslab_monitor.ko
cc -Wall -Wextra -std=c11 -o oslabctl oslabctl.c
oslab monitor integration tests passed
```

集成测试覆盖：

- 内核模块编译。
- `sudo insmod oslab_monitor.ko` 加载。
- `/proc/oslab_monitor/overview` 字段检查。
- `/proc/oslab_monitor/tasks` 表头检查。
- `/proc/oslab_monitor/pid` 写入和读取。
- `oslabctl overview`。
- `oslabctl tasks`。
- `sudo oslabctl pid 1`。
- `sudo rmmod oslab_monitor` 卸载。
- 卸载后确认 `/proc/oslab_monitor/` 不存在。

### 10.3 测试结论

基础四模块和扩展模块均已通过自动化测试。基础模块可在普通 Linux 命令行环境中验证，扩展模块已在 Ubuntu 24.04.2 VM 默认内核中完成验证。

## 11. 遇到的问题与处理

### 11.1 WSL2 无法作为扩展验收环境

在 WSL2 中执行扩展测试时，出现过如下问题：

```text
/lib/modules/5.15.123.1-microsoft-standard-WSL2/build: No such file or directory
```

原因是 WSL2 默认环境不一定提供当前内核的构建目录，也不适合作为内核模块加载验收环境。处理方式是将扩展模块放到 Ubuntu VM 中验证，并安装匹配当前内核的 headers。

### 11.2 Bash 脚本行尾问题

在 Windows 和 Linux 环境之间同步脚本时，脚本行尾如果变成 CRLF，Linux 下执行可能出现：

```text
set: pipefail: invalid option name
```

为避免这个问题，项目新增 `.gitattributes`：

```text
*.sh text eol=lf
Makefile text eol=lf
```

这样 Bash 脚本和 Makefile 在仓库中固定使用 LF 行尾，减少跨平台同步时的执行问题。

### 11.3 `pipefail` 与 `head` 截断问题

扩展脚本中如果写成：

```bash
cat /proc/oslab_monitor/tasks | head
```

在 `set -euo pipefail` 下，`head` 读够行数后退出，前面的 `cat` 可能收到 `SIGPIPE`，导致整个脚本被误判为失败。处理方式是改用临时文件或直接使用 `head -n` 读取文件，避免正常截断被当成错误。

### 11.4 内核线程 `mm == NULL`

读取进程 RSS 时，内核线程可能没有用户态地址空间，即 `mm == NULL`。如果不判断就访问，会有空指针风险。实现中使用 `get_task_mm()`，若返回 NULL，则 RSS 输出 0；若成功获取，读取后调用 `mmput()`。

## 12. 项目总结

本项目完成了操作系统课程设计中要求的基础模块和扩展模块。

基础部分通过四个独立命令行程序实现了调度、内存、同步和文件系统。每个模块都有自己的输入样例、Makefile 和测试脚本，能够单独编译和运行。实现时没有追求复杂功能，而是优先保证算法过程清楚、输出稳定、测试可重复。

扩展部分通过 Linux 内核模块实现了真实系统运行态观测。`overview` 可以查看系统概览，`tasks` 可以查看进程列表，`pid` 可以查询指定进程详情。用户态工具 `oslabctl` 简化了访问流程，同时保持输出与 `/proc` 原始内容一致。

通过这个项目，可以把课程中抽象的操作系统概念和实际 Linux 系统联系起来。例如，调度模块中的优先级、时间片和等待时间，可以和扩展模块中真实进程的调度策略、优先级、nice 值进行对照；内存管理中的页面和缺页概念，也可以和真实进程的 RSS 信息联系起来。

整体来看，项目达到了以下目标：

- 基础四模块全部实现并通过测试。
- 扩展内核模块可编译、加载、读取和卸载。
- `/proc/oslab_monitor/` 三个接口字段稳定。
- `oslabctl` 可正常访问扩展接口。
- 文档、运行命令和测试结果已同步。

## 13. 参考资料

- Linux Kernel Documentation
- Linux `proc_fs` 与 `seq_file` 相关文档
- POSIX pthread 文档
- GNU Make 文档
- 操作系统课程中关于调度、内存管理、同步和文件系统的教材内容
