# PRD：操作系统课程设计项目

## 1. 项目概述

本项目用于完成《操作系统》课程设计，采用“基础必做部分 + 自由扩展提升部分”的两级实现方式。基础必做部分覆盖处理机调度、内存管理、进程同步与并发控制、文件系统四个核心模块；自由扩展提升部分选择“Linux 内核与系统编程”方向，设计并实现一个基于 `/proc` 接口的 Linux 进程与内存运行态观测模块。

项目必须将基础部分和扩展部分分开实现、分开存放，避免不同实验目标、代码依赖和运行环境互相混杂。基础部分使用 C 语言命令行程序实现，面向操作系统核心算法和机制模拟；扩展部分使用 Linux 内核模块和用户态命令行工具实现，面向真实 Linux 系统级编程实践。

## 2. 项目目标

- 完成课程设计基础必做部分的四个模块，覆盖处理机调度、内存管理、进程同步与并发控制、文件系统。
- 完成自由扩展提升部分，构建可编译、可加载、可卸载的 Linux 内核模块 `oslab_monitor.ko`。
- 在扩展部分通过 `/proc/oslab_monitor/` 暴露系统运行态信息，包括系统概览、进程列表和指定 PID 详情。
- 提供用户态命令行工具 `oslabctl`，封装对 `/proc/oslab_monitor/` 的读取和写入操作。
- 提供清晰的编译、运行、测试和报告材料，满足课程提交要求中的代码 URL、访问方式、运行环境、实验截图和结果分析。

## 3. 用户与使用场景

目标用户为完成操作系统课程设计的学生、助教或课程验收人员。

主要使用场景：

- 学生运行基础模块程序，输入进程、内存、同步或文件系统相关参数，观察算法执行过程和统计结果。
- 学生加载 Linux 内核模块，通过 `/proc` 接口观察真实 Linux 系统中的进程、调度和内存信息。
- 学生使用 `oslabctl` 工具快速查看系统概览、进程列表和指定 PID 详情。
- 助教或教师根据 GitHub 仓库、运行说明、截图和报告验证项目是否满足课程设计要求。

## 4. 范围划分

### 4.1 基础必做部分

基础必做部分存放在 `basic/` 目录下，使用 C 语言实现四个独立模块：

- 处理机调度模块。
- 内存管理模块。
- 进程同步与并发控制模块。
- 文件系统模块。

基础部分重点体现操作系统核心知识与基本实现能力，要求支持动态输入、输出关键过程和统计结果，并能够为实验报告提供可分析的数据。

### 4.2 自由扩展提升部分

自由扩展提升部分存放在 `extension/` 目录下，使用 C 语言实现 Linux 内核模块和用户态命令行工具：

- 内核模块：`oslab_monitor.ko`。
- `/proc` 接口目录：`/proc/oslab_monitor/`。
- 用户态工具：`oslabctl`。

扩展部分重点体现 Linux 内核模块开发、`/proc` 接口开发、内核态与用户态交互、真实系统运行态观测能力。

## 5. 推荐目录结构

```text
OS-Design/
├── basic/
│   ├── scheduler/
│   │   ├── Makefile
│   │   ├── include/
│   │   ├── src/
│   │   └── tests/
│   ├── memory/
│   │   ├── Makefile
│   │   ├── include/
│   │   ├── src/
│   │   └── tests/
│   ├── sync/
│   │   ├── Makefile
│   │   ├── include/
│   │   ├── src/
│   │   └── tests/
│   └── filesystem/
│       ├── Makefile
│       ├── include/
│       ├── src/
│       └── tests/
├── extension/
│   └── oslab_monitor/
│       ├── kernel/
│       │   ├── oslab_monitor.c
│       │   └── Makefile
│       ├── user/
│       │   ├── oslabctl.c
│       │   └── Makefile
│       ├── scripts/
│       │   ├── load.sh
│       │   ├── unload.sh
│       │   └── demo.sh
│       └── tests/
│           └── test_oslab_monitor.sh
├── docs/
│   ├── PRD.md
│   ├── TECH.md
│   ├── FEATURES.md
│   └── prompts/
└── README.md
```

目录约束：

- `basic/` 只存放基础必做部分代码和测试。
- `extension/` 只存放自由扩展提升部分代码和测试。
- `docs/` 存放 PRD、报告草稿、实验截图说明和课程要求相关材料。
- 根目录 `README.md` 必须说明项目结构、运行环境、编译方式、运行方式和代码访问方式。

### 5.1 全局实现约定

为避免基础模块在实现和验收时出现不同理解，本项目采用以下统一约定：

- 基础必做部分固定实现为四个独立命令行程序，不采用统一总入口程序。
- 四个基础程序名称分别为 `scheduler`、`memory`、`sync`、`filesystem`。
- 每个基础模块必须提供独立 `Makefile`，支持 `make`、`make clean`。
- 每个基础模块必须采用 `include/`、`src/`、`tests/` 的内部目录结构。
- 每个基础程序必须支持 `--help`，用于输出参数说明和输入格式说明。
- 需要批处理输入的程序必须从标准输入读取文本命令或样例文件。
- 所有程序正常完成时返回退出码 `0`。
- 参数缺失、输入格式错误、非法数值、资源不足等错误场景必须输出以 `error:` 开头的提示，并返回非 `0` 退出码。
- 所有数值输入默认采用十进制整数；除非单项需求特别说明，数量、时间、大小、页号、块号均不得为负数。
- 输出字段名应保持稳定，便于测试脚本、报告截图和结果比对。
- 输出中的平均值、比率和带权周转时间默认保留两位小数。
- 空行可以忽略；是否支持注释行由各模块自行决定，但必须在 `--help` 中说明。
- `docs/PRD.md` 描述产品需求，`docs/TECH.md` 描述实现级技术方案，`docs/FEATURES.md` 描述用户可见功能清单；三者必须保持一致。

## 6. 技术栈与运行环境

- 编程语言：C。
- 基础部分运行方式：Linux 命令行程序。
- 扩展部分运行方式：Ubuntu 虚拟机中的 Linux 内核模块。
- 推荐系统：Ubuntu 22.04 LTS 或 Ubuntu 24.04 LTS 虚拟机。
- 基础工具链：`gcc`、`make`、`pthread`、`bash`。
- 内核模块工具链：`build-essential`、`linux-headers-$(uname -r)`、`kbuild`、`insmod`、`rmmod`、`lsmod`、`dmesg`。

环境约束：

- 不建议将扩展部分作为 WSL2 环境下的默认实现目标，因为 WSL2 内核模块编译和加载存在额外限制。
- 扩展部分必须在真实 Linux 虚拟机或可加载内核模块的 Linux 环境中验证。
- 所有命令行程序应提供 `Makefile`，支持 `make` 编译和 `make clean` 清理。

## 7. 基础必做部分需求

### 7.1 基础部分总体要求

基础必做部分必须实现为四个独立命令行程序，每个模块必须可以单独编译、单独运行、单独测试。

基础部分统一要求：

- 必须使用 C 语言实现。
- 必须支持动态输入，不能只写死固定样例。
- 必须输出执行过程、关键中间状态和最终统计结果。
- 必须提供示例输入文件或示例运行命令。
- 必须提供基础测试用例，覆盖正常输入、边界输入和典型错误输入。
- 必须在报告中解释核心算法思想、数据结构、实验结果和不同方案的差异。

基础部分命令行入口固定为：

```bash
basic/scheduler/scheduler
basic/memory/memory
basic/sync/sync
basic/filesystem/filesystem
```

### 7.2 处理机调度模块

模块目录：`basic/scheduler/`

目标：模拟典型处理机调度算法，输出进程运行顺序、等待时间、周转时间、带权周转时间等结果，并分析不同调度算法的性能差异。

#### 功能需求

- B-SCH-FR-1：系统必须支持动态输入进程数量。
- B-SCH-FR-2：系统必须支持输入每个进程的进程名或 PID、到达时间、服务时间。
- B-SCH-FR-3：系统必须支持输入时间片长度，用于时间片轮转算法。
- B-SCH-FR-4：系统必须支持输入进程优先级，用于优先级调度算法。
- B-SCH-FR-5：系统必须实现先来先服务算法 FCFS。
- B-SCH-FR-6：系统必须实现短作业优先算法 SJF，默认采用非抢占式 SJF。
- B-SCH-FR-7：系统必须实现时间片轮转算法 RR。
- B-SCH-FR-8：系统必须实现优先级调度算法，默认采用非抢占式优先级调度。
- B-SCH-FR-9：系统必须输出每种算法的进程执行顺序。
- B-SCH-FR-10：系统必须输出每个进程的开始时间、完成时间、等待时间、周转时间、带权周转时间。
- B-SCH-FR-11：系统必须输出每种算法的平均等待时间、平均周转时间、平均带权周转时间。
- B-SCH-FR-12：系统必须支持通过命令行参数选择调度算法。

#### 命令行与算法约定

调度模块命令行格式：

```bash
./scheduler --algorithm fcfs|sjf|rr|priority < tests/sample.txt
```

调度模块输入规则：

- 第一行必须包含 `process_count = N`，其中 `N > 0`。
- RR 算法必须包含 `time_quantum = Q`，其中 `Q > 0`。
- 每个进程一行，格式为 `NAME arrival=A burst=B priority=P`。
- `NAME` 为不含空格的进程名或 PID 字符串，最大长度为 31 个字符。
- `arrival` 必须为非负整数，`burst` 必须为正整数。
- `priority` 必须为整数，数值越小表示优先级越高。

调度算法统一规则：

- FCFS 按到达时间排序；到达时间相同时按输入顺序排序。
- SJF 默认采用非抢占式 SJF，只在 CPU 空闲并选择下一个进程时比较已到达进程的服务时间。
- SJF 服务时间相同时，按到达时间排序；仍相同时按输入顺序排序。
- RR 使用普通就绪队列；时间片结束但未完成的进程排到当前已到达就绪队列末尾。
- RR 中同一时刻新到达进程按输入顺序入队。
- 非抢占式优先级调度只在 CPU 空闲并选择下一个进程时比较已到达进程优先级。
- 优先级相同时，按到达时间排序；仍相同时按输入顺序排序。
- 当没有进程可运行但仍有进程未到达时，执行序列必须显示 `IDLE` 时间段。

#### 输入示例

```text
process_count = 4
time_quantum = 2
P1 arrival=0 burst=5 priority=2
P2 arrival=1 burst=3 priority=1
P3 arrival=2 burst=8 priority=4
P4 arrival=3 burst=6 priority=3
```

#### 输出要求

输出必须包含：

- 算法名称。
- 甘特图或等价的执行序列。
- 每个进程的统计表。
- 全局平均指标。

#### 验收标准

- B-SCH-AC-1：给定固定输入时，FCFS、SJF、RR、优先级调度均能输出正确执行序列。
- B-SCH-AC-2：统计表中每个进程的完成时间、等待时间、周转时间计算正确。
- B-SCH-AC-3：当进程到达时间不全为 0 时，算法仍能正确处理 CPU 空闲时间。
- B-SCH-AC-4：当 RR 时间片为 1、2、4 等不同值时，输出结果随时间片变化。
- B-SCH-AC-5：非法输入能够给出错误提示，不发生崩溃。

### 7.3 内存管理模块

模块目录：`basic/memory/`

目标：模拟动态分区管理和页面置换机制，展示内存分配、回收、页面淘汰过程，并统计缺页次数和缺页率。

#### 功能需求

- B-MEM-FR-1：系统必须支持设置总内存大小。
- B-MEM-FR-2：系统必须支持动态输入作业或进程的内存申请大小。
- B-MEM-FR-3：系统必须实现首次适应算法 FF。
- B-MEM-FR-4：系统必须实现最佳适应算法 BF。
- B-MEM-FR-5：系统必须支持内存分配操作。
- B-MEM-FR-6：系统必须支持内存回收操作。
- B-MEM-FR-7：系统必须在回收内存后合并相邻空闲分区。
- B-MEM-FR-8：系统必须展示每次分配或回收后的空闲分区表和已分配分区表。
- B-MEM-FR-9：系统必须支持设置物理页框数量。
- B-MEM-FR-10：系统必须支持动态输入页面访问序列。
- B-MEM-FR-11：系统必须实现 FIFO 页面置换算法。
- B-MEM-FR-12：系统必须实现 LRU 页面置换算法。
- B-MEM-FR-13：系统必须输出每次页面访问后的页框状态。
- B-MEM-FR-14：系统必须统计缺页次数和缺页率。

#### 命令行与算法约定

内存模块必须支持两种运行模式：

```bash
./memory --mode partition --algorithm ff|bf < tests/partition.txt
./memory --mode paging --algorithm fifo|lru < tests/pages.txt
```

动态分区模式输入规则：

- 第一行必须包含 `memory_size = SIZE`，其中 `SIZE > 0`。
- 分配命令格式为 `alloc NAME SIZE`，其中 `NAME` 为不含空格的作业或进程名，最大长度为 31 个字符，`SIZE > 0`。
- 回收命令格式为 `free NAME`。
- 同一 `NAME` 重复分配时必须报错。
- 回收不存在的 `NAME` 时必须报错。

动态分区算法规则：

- 虚拟内存地址从 `0` 开始。
- 空闲分区表和已分配分区表必须按起始地址升序输出。
- FF 从低地址到高地址选择第一个足够大的空闲分区。
- BF 选择能够容纳申请且剩余空间最小的空闲分区；剩余空间相同时选择起始地址更小的分区。
- 分配后若空闲分区仍有剩余空间，必须拆分分区。
- 回收后必须合并相邻空闲分区。

页面置换模式输入规则：

- 第一行必须包含 `frame_count = N`，其中 `N > 0`。
- 第二行必须包含 `reference_string = ...`，访问序列必须至少包含一个非负整数页号。

页面置换算法规则：

- 初始页框为空。
- 页框状态按页框下标从小到大输出。
- 命中时 `evicted` 字段输出 `-`。
- 未发生淘汰的缺页也输出 `evicted = -`。
- 缺页率按 `缺页次数 / 页面访问次数 * 100` 计算，保留两位小数并带 `%`。

#### 输入示例

动态分区输入示例：

```text
memory_size = 640
alloc P1 130
alloc P2 60
alloc P3 100
free P2
alloc P4 50
```

页面置换输入示例：

```text
frame_count = 3
reference_string = 7 0 1 2 0 3 0 4 2 3 0 3 2
```

#### 输出要求

动态分区输出必须包含：

- 操作编号。
- 当前操作内容。
- 分配是否成功。
- 已分配分区表。
- 空闲分区表。

页面置换输出必须包含：

- 页面访问序列。
- 每一步访问后页框内容。
- 是否缺页。
- 被淘汰页面。
- 缺页次数。
- 缺页率。

#### 验收标准

- B-MEM-AC-1：FF 和 BF 对同一输入能够产生符合算法定义的不同分区选择结果。
- B-MEM-AC-2：释放分区后，相邻空闲分区能够正确合并。
- B-MEM-AC-3：FIFO 和 LRU 能够正确统计缺页次数。
- B-MEM-AC-4：页面访问序列为空、页框数量非法、内存申请大于总内存时，程序能给出错误提示。
- B-MEM-AC-5：输出过程足够清晰，可直接用于报告截图和分析。

### 7.4 进程同步与并发控制模块

模块目录：`basic/sync/`

目标：使用多线程、互斥锁和信号量模拟经典同步问题，展示并发执行、互斥访问、条件同步和死锁规避。

#### 功能需求

- B-SYNC-FR-1：系统必须使用 POSIX 线程 `pthread` 实现多线程模拟。
- B-SYNC-FR-2：系统必须使用互斥锁、条件变量或信号量实现同步控制。
- B-SYNC-FR-3：系统必须实现生产者-消费者问题。
- B-SYNC-FR-4：生产者-消费者问题必须支持设置生产者数量、消费者数量、缓冲区大小、生产次数。
- B-SYNC-FR-5：系统必须实现读者-写者问题。
- B-SYNC-FR-6：读者-写者问题必须支持设置读者数量、写者数量、读写次数。
- B-SYNC-FR-7：系统必须实现哲学家进餐问题。
- B-SYNC-FR-8：哲学家进餐问题必须支持 5 个哲学家和 5 支筷子的经典场景。
- B-SYNC-FR-9：系统必须输出每个线程的关键状态变化。
- B-SYNC-FR-10：系统必须避免数据竞争。
- B-SYNC-FR-11：系统必须避免死锁。

#### 命令行与运行约定

同步模块命令行格式：

```bash
./sync --problem producer_consumer --producers 2 --consumers 2 --buffer-size 4 --count 10
./sync --problem readers_writers --readers 3 --writers 2 --count 5
./sync --problem dining_philosophers --count 3
```

同步模块默认参数：

- 生产者-消费者：`producers = 2`，`consumers = 2`，`buffer-size = 4`，`count = 10`。
- 读者-写者：`readers = 3`，`writers = 2`，`count = 5`。
- 哲学家进餐：固定 `5` 个哲学家和 `5` 支筷子，默认每个哲学家进餐 `count = 3` 次。

同步模块运行规则：

- 生产者-消费者中，`count` 表示每个生产者的生产次数；消费者整体消费目标为 `producers * count`，完成后所有消费者退出。
- 读者-写者中，`count` 表示每个读者或写者的目标操作次数。
- 哲学家进餐中，`count` 表示每个哲学家的目标进餐次数。
- 程序必须在所有线程完成目标次数后自动退出。
- 默认参数下程序必须在 3 秒内结束。
- 哲学家进餐采用“最多 4 个哲学家同时尝试拿筷子”的死锁规避策略，并在报告中说明。
- 正常完成时必须输出汇总统计，并返回退出码 `0`。
- 若参数非法，必须输出 `error:` 提示并返回非 `0` 退出码。

#### 输出要求

输出必须包含：

- 线程编号。
- 当前动作。
- 缓冲区状态或共享资源状态。
- 加锁、等待、唤醒、释放资源等关键事件。
- 程序结束时的汇总统计。

不同问题的汇总统计必须至少包含：

- 生产者-消费者：`produced_total`、`consumed_total`、`buffer_final_size`。
- 读者-写者：`read_total`、`write_total`、`final_shared_value`。
- 哲学家进餐：每个哲学家的 `eat_count`，以及 `deadlock_detected: no`。

#### 验收标准

- B-SYNC-AC-1：生产者-消费者在缓冲区满时阻塞生产者，在缓冲区空时阻塞消费者。
- B-SYNC-AC-2：读者-写者能够保证写者写入时没有其他读者或写者访问共享数据。
- B-SYNC-AC-3：哲学家进餐问题能够运行完整轮次并结束，不发生死锁。
- B-SYNC-AC-4：多次运行不会出现明显的数据竞争、资源计数错误或线程无法退出。
- B-SYNC-AC-5：输出日志能说明同步机制的作用。

### 7.5 文件系统模块

模块目录：`basic/filesystem/`

目标：设计并实现一个简易文件系统或目录管理模块，支持文件创建、读写、删除、目录管理和空闲空间管理。

#### 功能需求

- B-FS-FR-1：系统必须模拟一个固定大小的虚拟磁盘。
- B-FS-FR-2：系统必须将虚拟磁盘划分为固定大小的数据块。
- B-FS-FR-3：系统必须实现空闲块管理。
- B-FS-FR-4：系统必须支持文件创建。
- B-FS-FR-5：系统必须支持文件写入。
- B-FS-FR-6：系统必须支持文件读取。
- B-FS-FR-7：系统必须支持文件删除。
- B-FS-FR-8：系统必须支持目录创建。
- B-FS-FR-9：系统必须支持目录列表展示。
- B-FS-FR-10：系统必须维护文件元数据，包括文件名、大小、起始块或块列表、创建时间或修改时间。
- B-FS-FR-11：系统必须在文件删除后释放其占用的数据块。
- B-FS-FR-12：系统必须提供交互式命令或批处理命令执行方式。

#### 命令行与文件系统约定

文件系统模块命令行格式：

```bash
./filesystem < tests/fs_commands.txt
```

文件系统实现边界：

- 虚拟磁盘只要求在程序运行期间保存在内存中，不要求持久化到真实磁盘文件。
- 必须支持绝对路径，路径以 `/` 开头。
- 必须支持多级目录，例如 `/docs/os/a.txt`。
- 文件名、目录名只能包含字母、数字、下划线、短横线和点号，最大长度为 31 个字符。
- 路径最大长度为 255 个字符。
- 文件内容固定为不含空格的字符串。
- `write PATH CONTENT` 默认覆盖原文件内容，不要求追加写。
- 空闲块管理采用块位图。
- 文件块分配采用块列表方式，不要求连续分配。
- 删除文件必须释放其占用块。
- 不要求支持删除目录；若实现 `rmdir`，非空目录必须拒绝删除并提示错误。

文件系统命令规则：

- `mkfs DISK_SIZE BLOCK_SIZE`：初始化虚拟磁盘，`DISK_SIZE` 和 `BLOCK_SIZE` 必须为正整数，且 `DISK_SIZE` 必须能被 `BLOCK_SIZE` 整除。
- `mkdir PATH`：创建目录。
- `create PATH`：创建空文件。
- `write PATH CONTENT`：覆盖写入文件内容。
- `read PATH`：读取文件内容。
- `ls PATH`：列出目录内容。
- `delete PATH`：删除文件。
- `stat`：输出文件系统整体状态。
- 重复创建同名文件或目录、读取不存在文件、删除不存在文件、空间不足时，必须输出 `error:` 提示。

#### 命令示例

```text
mkfs 1024 64
mkdir /docs
create /docs/a.txt
write /docs/a.txt hello_os
read /docs/a.txt
ls /docs
delete /docs/a.txt
stat
```

#### 输出要求

输出必须包含：

- 命令执行结果。
- 文件或目录是否存在。
- 文件大小和占用块信息。
- 空闲块数量。
- 文件系统整体状态。

#### 验收标准

- B-FS-AC-1：创建文件后，目录列表中能看到对应文件。
- B-FS-AC-2：写入文件后，读取结果与写入内容一致。
- B-FS-AC-3：删除文件后，其占用块被释放，目录列表中不再显示该文件。
- B-FS-AC-4：磁盘空间不足时，系统给出明确错误提示。
- B-FS-AC-5：重复创建同名文件、读取不存在文件、删除不存在文件时，系统能给出明确错误提示。

## 8. 自由扩展提升部分需求

### 8.1 扩展部分总体要求

扩展部分选择课程可选方向中的“Linux 内核与系统编程”，具体实现为基于 `/proc` 接口的 Linux 进程与内存运行态观测系统。

扩展部分必须与基础部分独立存放，目录为 `extension/oslab_monitor/`。扩展部分不依赖基础部分代码，基础部分也不依赖扩展部分代码。

扩展部分统一要求：

- 必须实现可编译的 Linux 内核模块。
- 必须实现可加载和可卸载流程。
- 必须通过 `/proc/oslab_monitor/` 暴露观测接口。
- 必须提供用户态命令行工具 `oslabctl`。
- 必须提供编译、加载、运行、卸载、清理命令说明。
- 必须提供实验截图或命令输出，用于课程报告。

扩展部分兼容性约定：

- 目标验证环境为 Ubuntu 22.04 LTS 或 Ubuntu 24.04 LTS 的默认发行版内核。
- 报告中必须记录实际验证环境的 `uname -a`、`gcc --version` 和 `lsb_release -a` 或等价信息。
- 若不同内核版本导致某些 `task_struct`、`mm_struct` 或缺页统计字段不可直接访问，允许使用等价字段或输出 `N/A`，但字段名和字段顺序必须保持稳定。
- 访问进程内存信息时必须处理内核线程或无地址空间进程的 `mm == NULL` 情况。
- RSS 可采用 `get_mm_rss(mm)`、`get_task_mm()` 配合安全释放，或当前内核推荐的等价方式。
- 所有 `/proc` 输出必须避免固定小缓冲区截断长内容。

### 8.2 扩展部分目录结构

```text
extension/oslab_monitor/
├── kernel/
│   ├── oslab_monitor.c
│   └── Makefile
├── user/
│   ├── oslabctl.c
│   └── Makefile
├── scripts/
│   ├── load.sh
│   ├── unload.sh
│   └── demo.sh
└── tests/
    └── test_oslab_monitor.sh
```

目录约束：

- `kernel/` 只存放内核模块源码和内核模块编译文件。
- `user/` 只存放用户态工具源码和编译文件。
- `scripts/` 存放演示、加载、卸载脚本。
- `tests/` 存放测试脚本。

### 8.3 内核模块 `oslab_monitor.ko`

目标：实现 Linux 内核模块，加载后创建 `/proc/oslab_monitor/` 目录，并在该目录下创建 `overview`、`tasks`、`pid` 三个接口。

#### 功能需求

- E-KMOD-FR-1：内核模块源码文件名必须为 `oslab_monitor.c`。
- E-KMOD-FR-2：内核模块编译产物必须为 `oslab_monitor.ko`。
- E-KMOD-FR-3：模块加载时必须创建 `/proc/oslab_monitor/` 目录。
- E-KMOD-FR-4：模块加载时必须创建 `/proc/oslab_monitor/overview`。
- E-KMOD-FR-5：模块加载时必须创建 `/proc/oslab_monitor/tasks`。
- E-KMOD-FR-6：模块加载时必须创建 `/proc/oslab_monitor/pid`。
- E-KMOD-FR-7：模块卸载时必须删除所有创建的 `/proc` 文件和目录。
- E-KMOD-FR-8：模块加载和卸载时必须向内核日志输出明确日志，可通过 `dmesg` 查看。
- E-KMOD-FR-9：模块必须使用 `seq_file` 或等价的安全方式输出长文本内容。
- E-KMOD-FR-10：模块必须避免内核空指针访问、越界写入和明显资源泄露。
- E-KMOD-FR-11：`overview` 和 `tasks` 必须创建为只读权限 `0444`。
- E-KMOD-FR-12：`pid` 必须支持读写，权限固定为 `0644`，用户态写入示例必须使用 `sudo`。

#### 编译与运行命令

```bash
cd extension/oslab_monitor/kernel
make
sudo insmod oslab_monitor.ko
ls /proc/oslab_monitor
cat /proc/oslab_monitor/overview
cat /proc/oslab_monitor/tasks
sudo rmmod oslab_monitor
dmesg | tail
```

#### 验收标准

- E-KMOD-AC-1：执行 `make` 后生成 `oslab_monitor.ko`。
- E-KMOD-AC-2：执行 `sudo insmod oslab_monitor.ko` 后，`lsmod` 能看到模块。
- E-KMOD-AC-3：模块加载后，`/proc/oslab_monitor/` 目录存在。
- E-KMOD-AC-4：模块加载后，`overview`、`tasks`、`pid` 三个 `/proc` 文件存在。
- E-KMOD-AC-5：模块卸载后，`/proc/oslab_monitor/` 被清理。
- E-KMOD-AC-6：加载、读取、卸载流程不导致系统崩溃。

### 8.4 `/proc/oslab_monitor/overview`

目标：输出系统级概览信息，用于快速观察当前 Linux 系统运行态。

#### 功能需求

- E-OV-FR-1：读取 `/proc/oslab_monitor/overview` 时必须输出模块名称。
- E-OV-FR-2：必须输出当前内核版本或构建相关信息。
- E-OV-FR-3：必须输出当前系统总进程数量。
- E-OV-FR-4：必须输出运行态、睡眠态、停止态、僵尸态进程数量。
- E-OV-FR-5：必须输出系统总内存、空闲内存、可用内存。
- E-OV-FR-6：必须输出模块采样时间或读取时间。

#### 输出格式示例

```text
module: oslab_monitor
kernel: 6.x.x
total_tasks: 156
running_tasks: 2
sleeping_tasks: 151
stopped_tasks: 0
zombie_tasks: 0
mem_total_kb: 8045120
mem_free_kb: 1234560
mem_available_kb: 4567890
read_time_jiffies: 4294891234
```

#### 验收标准

- E-OV-AC-1：执行 `cat /proc/oslab_monitor/overview` 能输出非空内容。
- E-OV-AC-2：输出字段名稳定，便于 `oslabctl` 解析。
- E-OV-AC-3：进程数量和内存数值为非负数。
- E-OV-AC-4：连续读取多次不导致内核日志出现错误。

### 8.5 `/proc/oslab_monitor/tasks`

目标：输出当前系统中的进程列表，展示进程、调度和内存相关字段。

#### 功能需求

- E-TASK-FR-1：读取 `/proc/oslab_monitor/tasks` 时必须输出表头。
- E-TASK-FR-2：每个进程至少输出 PID。
- E-TASK-FR-3：每个进程至少输出进程名。
- E-TASK-FR-4：每个进程至少输出进程状态。
- E-TASK-FR-5：每个进程至少输出调度策略。
- E-TASK-FR-6：每个进程至少输出优先级或 nice 值。
- E-TASK-FR-7：每个进程至少输出线程数量。
- E-TASK-FR-8：每个进程至少输出 RSS 或等价内存占用字段。
- E-TASK-FR-9：每个进程至少输出主缺页次数和次缺页次数，若当前内核字段访问受限，必须在报告中说明替代字段。
- E-TASK-FR-10：输出必须能够容纳较多进程，不允许只输出固定数量。

#### 输出格式示例

```text
PID     COMM            STATE   POLICY  PRIO  NICE  THREADS  RSS_KB  MIN_FLT  MAJ_FLT
1       systemd         S       NORMAL  120   0     1        12600   1024     2
1024    bash            S       NORMAL  120   0     1        4800    300      0
```

#### 验收标准

- E-TASK-AC-1：执行 `cat /proc/oslab_monitor/tasks` 能输出表头和至少一个进程。
- E-TASK-AC-2：输出中能找到当前 shell 或测试进程。
- E-TASK-AC-3：输出字段顺序稳定。
- E-TASK-AC-4：当系统进程数量较多时，输出不截断为固定小样本。
- E-TASK-AC-5：连续读取多次不导致内核错误。

### 8.6 `/proc/oslab_monitor/pid`

目标：支持用户写入目标 PID，然后读取该 PID 的详细信息。

#### 功能需求

- E-PID-FR-1：系统必须允许向 `/proc/oslab_monitor/pid` 写入一个 PID。
- E-PID-FR-2：写入 PID 后，再读取 `/proc/oslab_monitor/pid` 必须输出该 PID 对应进程的详细信息。
- E-PID-FR-3：详细信息至少包含 PID、进程名、状态、父进程 PID、调度策略、优先级、nice 值、线程数、内存占用。
- E-PID-FR-4：当 PID 不存在时，读取结果必须给出明确提示。
- E-PID-FR-5：当写入内容不是合法整数 PID 时，写入操作必须返回错误或在读取时给出明确提示。
- E-PID-FR-6：写入 PID 时必须限制输入长度，避免缓冲区溢出。

#### 使用示例

```bash
echo 1 | sudo tee /proc/oslab_monitor/pid
cat /proc/oslab_monitor/pid
```

#### 输出格式示例

```text
pid: 1
comm: systemd
state: S
ppid: 0
policy: NORMAL
prio: 120
nice: 0
threads: 1
rss_kb: 12600
```

#### 验收标准

- E-PID-AC-1：写入存在的 PID 后，读取能输出对应进程详情。
- E-PID-AC-2：写入不存在的 PID 后，读取能输出 `not found` 或等价提示。
- E-PID-AC-3：写入非法内容时，模块不会崩溃。
- E-PID-AC-4：连续写入不同 PID 后，读取结果能随目标 PID 改变。

### 8.7 用户态工具 `oslabctl`

模块目录：`extension/oslab_monitor/user/`

目标：实现用户态命令行工具，封装 `/proc/oslab_monitor/` 的常用操作，降低验收和演示时的操作复杂度。

#### 功能需求

- E-CLI-FR-1：用户态工具源码文件名必须为 `oslabctl.c`。
- E-CLI-FR-2：执行 `make` 后必须生成可执行文件 `oslabctl`。
- E-CLI-FR-3：`oslabctl overview` 必须读取并输出 `/proc/oslab_monitor/overview`。
- E-CLI-FR-4：`oslabctl tasks` 必须读取并输出 `/proc/oslab_monitor/tasks`。
- E-CLI-FR-5：`oslabctl pid <PID>` 必须向 `/proc/oslab_monitor/pid` 写入 PID，并读取该 PID 详情。
- E-CLI-FR-6：`oslabctl --help` 必须输出使用说明。
- E-CLI-FR-7：当内核模块未加载时，工具必须给出明确错误提示。
- E-CLI-FR-8：当参数缺失或错误时，工具必须给出明确错误提示。
- E-CLI-FR-9：当写入 `/proc/oslab_monitor/pid` 因权限不足失败时，工具必须提示使用 `sudo` 或检查 `/proc` 文件权限。

#### 用户态工具错误处理约定

`oslabctl` 必须对以下错误场景输出明确提示并返回非 `0` 退出码：

- `/proc/oslab_monitor/` 不存在：提示内核模块未加载。
- `/proc/oslab_monitor/overview`、`tasks` 或 `pid` 打开失败：输出具体文件路径和系统错误原因。
- `pid` 参数缺失、不是整数或小于等于 `0`：输出参数错误和帮助提示。
- 写入 `/proc/oslab_monitor/pid` 失败且错误为权限不足：提示使用 `sudo ./oslabctl pid <PID>`。

#### 命令示例

```bash
cd extension/oslab_monitor/user
make
./oslabctl --help
./oslabctl overview
./oslabctl tasks
sudo ./oslabctl pid 1
```

#### 验收标准

- E-CLI-AC-1：`oslabctl overview` 输出内容与直接读取 `/proc/oslab_monitor/overview` 一致。
- E-CLI-AC-2：`oslabctl tasks` 输出内容与直接读取 `/proc/oslab_monitor/tasks` 一致。
- E-CLI-AC-3：`oslabctl pid 1` 能输出 PID 1 的详情。
- E-CLI-AC-4：模块未加载时，工具提示用户先加载模块。
- E-CLI-AC-5：参数错误时，工具输出帮助信息或错误说明。

## 9. 用户故事

### US-001：运行处理机调度实验

描述：作为学生，我希望输入一组进程参数并选择调度算法，以便观察不同处理机调度算法的执行顺序和统计指标。

验收标准：

- 可以输入进程数量、到达时间、服务时间和优先级。
- 可以选择 FCFS、SJF、RR、优先级调度。
- 可以输出执行序列、每个进程统计表和平均指标。
- 非法输入有错误提示。

### US-002：运行内存管理实验

描述：作为学生，我希望模拟动态分区管理和页面置换，以便理解内存分配、回收、缺页和页面淘汰机制。

验收标准：

- 可以运行 FF 和 BF 动态分区算法。
- 可以运行 FIFO 和 LRU 页面置换算法。
- 可以输出每一步内存状态或页框状态。
- 可以统计缺页次数和缺页率。

### US-003：运行进程同步实验

描述：作为学生，我希望通过多线程程序运行经典同步问题，以便观察互斥锁、信号量和条件同步的作用。

验收标准：

- 可以运行生产者-消费者问题。
- 可以运行读者-写者问题。
- 可以运行哲学家进餐问题。
- 程序运行结束后没有死锁或线程无法退出问题。

### US-004：运行简易文件系统实验

描述：作为学生，我希望通过命令操作一个模拟文件系统，以便理解文件创建、读写、删除、目录管理和空闲空间管理。

验收标准：

- 可以创建、写入、读取和删除文件。
- 可以创建目录并列出目录内容。
- 可以展示空闲块数量和文件占用块信息。
- 错误命令有明确提示。

### US-005：加载 Linux 内核观测模块

描述：作为学生，我希望加载一个 Linux 内核模块，以便通过真实 Linux 系统理解内核模块生命周期和 `/proc` 接口。

验收标准：

- `make` 能生成 `oslab_monitor.ko`。
- `insmod` 后 `/proc/oslab_monitor/` 存在。
- `rmmod` 后 `/proc/oslab_monitor/` 被清理。
- `dmesg` 能看到加载和卸载日志。

### US-006：查看系统运行态概览

描述：作为学生，我希望查看系统总进程数量、进程状态分布和内存概览，以便把课程中的进程和内存概念连接到真实 Linux 系统。

验收标准：

- `cat /proc/oslab_monitor/overview` 能输出系统概览。
- `oslabctl overview` 能输出同样信息。
- 输出字段稳定，可用于报告截图。

### US-007：查看进程列表与指定 PID 信息

描述：作为学生，我希望查看当前系统进程列表，并查询指定 PID 的详细信息，以便理解 `task_struct`、调度字段和内存字段的实际表现。

验收标准：

- `cat /proc/oslab_monitor/tasks` 能输出进程表。
- `oslabctl tasks` 能输出进程表。
- `oslabctl pid <PID>` 能输出指定进程详情。
- PID 不存在或非法时有明确提示。

## 10. 非目标与范围边界

本项目不包含以下内容：

- 不修改 Linux 内核源码。
- 不重新编译或替换 Linux 内核。
- 不实现真实 Linux 调度器修改。
- 不实现完整生产级文件系统。
- 不实现 GUI 图形界面。
- 不实现 Web 服务或前端页面。
- 不实现字符设备、`ioctl`、`debugfs`、`sysfs` 作为必做内容。
- 不要求支持 Windows 原生环境运行扩展部分。
- 不要求支持 WSL2 作为扩展部分默认验收环境。

可选增强但非必做：

- 为基础部分增加 CSV 输出。
- 为基础部分增加自动化测试脚本。
- 为扩展部分增加 benchmark。
- 为扩展部分增加字符设备 `/dev/oslab_ctl`。
- 为实验报告增加图表可视化。

## 11. 测试要求

### 11.1 基础部分测试

基础部分每个模块必须至少提供 3 类测试：

- 正常输入测试：验证主要功能可运行。
- 边界输入测试：验证空输入、极小值、极大值或临界状态。
- 错误输入测试：验证非法参数不会导致程序崩溃。

基础部分建议测试命令：

```bash
cd basic/scheduler && make && ./scheduler --algorithm fcfs < tests/sample.txt
cd basic/memory && make && ./memory --mode paging --algorithm lru < tests/pages.txt
cd basic/sync && make && ./sync --problem producer_consumer
cd basic/filesystem && make && ./filesystem < tests/fs_commands.txt
```

### 11.2 扩展部分测试

扩展部分必须测试：

- 内核模块能否编译。
- 内核模块能否加载。
- `/proc/oslab_monitor/overview` 是否可读。
- `/proc/oslab_monitor/tasks` 是否可读。
- `/proc/oslab_monitor/pid` 是否支持写入和读取。
- `oslabctl` 是否能调用所有目标命令。
- 内核模块能否卸载并清理 `/proc` 文件。

扩展部分建议测试命令：

```bash
cd extension/oslab_monitor/kernel
make
sudo insmod oslab_monitor.ko
cat /proc/oslab_monitor/overview
cat /proc/oslab_monitor/tasks | head
echo 1 | sudo tee /proc/oslab_monitor/pid
cat /proc/oslab_monitor/pid
cd ../user
make
./oslabctl overview
./oslabctl tasks
sudo ./oslabctl pid 1
sudo rmmod oslab_monitor
```

### 11.3 测试输出留存

项目必须保留以下材料用于报告：

- 基础调度模块运行截图或输出文本。
- 基础内存模块运行截图或输出文本。
- 基础同步模块运行截图或输出文本。
- 基础文件系统模块运行截图或输出文本。
- 扩展部分模块编译截图或输出文本。
- 扩展部分模块加载、读取 `/proc`、卸载截图或输出文本。
- `oslabctl` 运行截图或输出文本。

### 11.4 固定样例预期结果

为减少验收时对“正确结果”的主观判断，以下样例必须作为基础测试用例的一部分。

#### 调度模块固定样例

输入：

```text
process_count = 4
time_quantum = 2
P1 arrival=0 burst=5 priority=2
P2 arrival=1 burst=3 priority=1
P3 arrival=2 burst=8 priority=4
P4 arrival=3 burst=6 priority=3
```

预期执行序列：

- FCFS：`P1[0,5] P2[5,8] P3[8,16] P4[16,22]`
- SJF：`P1[0,5] P2[5,8] P4[8,14] P3[14,22]`
- Priority：`P1[0,5] P2[5,8] P4[8,14] P3[14,22]`
- RR，`time_quantum = 2`：`P1[0,2] P2[2,4] P3[4,6] P1[6,8] P4[8,10] P2[10,11] P3[11,13] P1[13,14] P4[14,16] P3[16,18] P4[18,20] P3[20,22]`

预期关键统计：

- FCFS 平均等待时间：`5.75`。
- FCFS 平均周转时间：`11.25`。
- SJF 平均等待时间：`5.25`。
- SJF 平均周转时间：`10.75`。
- RR 平均等待时间：`9.75`。
- RR 平均周转时间：`15.25`。

#### 页面置换固定样例

输入：

```text
frame_count = 3
reference_string = 7 0 1 2 0 3 0 4 2 3 0 3 2
```

预期结果：

- FIFO 缺页次数：`10`。
- FIFO 缺页率：`76.92%`。
- LRU 缺页次数：`9`。
- LRU 缺页率：`69.23%`。

#### 文件系统固定样例

输入：

```text
mkfs 1024 64
mkdir /docs
create /docs/a.txt
write /docs/a.txt hello_os
read /docs/a.txt
ls /docs
delete /docs/a.txt
ls /docs
stat
```

预期关键行为：

- `read /docs/a.txt` 输出必须包含 `hello_os`。
- `ls /docs` 在删除前必须包含 `a.txt`。
- `delete /docs/a.txt` 后再次列出 `/docs` 时不得包含 `a.txt`。
- `stat` 必须输出总块数、空闲块数和已用块数或等价字段。

#### 扩展部分固定检查

扩展部分测试脚本不应依赖具体进程数量或内存数值，但必须检查：

- `/proc/oslab_monitor/overview` 输出包含 `module:`、`kernel:`、`total_tasks:`、`mem_total_kb:`。
- `/proc/oslab_monitor/tasks` 输出包含稳定表头和至少一条进程记录。
- 向 `/proc/oslab_monitor/pid` 写入 `1` 后，读取结果包含 `pid:` 和 `comm:`。
- 模块卸载后 `/proc/oslab_monitor/` 不存在。

## 12. 性能与可靠性要求

- 基础部分所有命令行程序在正常输入下必须能在 3 秒内给出结果。
- 基础部分错误输入不能导致段错误。
- 同步模块在默认配置下必须能自动结束，不允许无限阻塞。
- 文件系统模块不能因用户输入过长而发生缓冲区溢出。
- 扩展部分读取 `/proc/oslab_monitor/tasks` 时不应明显卡死系统。
- 扩展部分必须保证模块卸载时清理所有由模块创建的 `/proc` 节点。
- 扩展部分必须避免在内核态使用不安全的固定长度写入方式。

## 13. 报告要求

最终课程报告必须包含以下内容：

- 项目名称。
- 小组成员及贡献说明，若为个人完成则说明个人完成。
- GitHub 或类似代码托管平台 URL。
- 代码访问方式说明。
- 运行环境说明，包括 Linux 发行版、内核版本、编译器版本。
- 项目目录结构说明。
- 基础必做部分四个模块的设计与实现说明。
- 基础必做部分四个模块的运行截图和结果分析。
- 自由扩展提升部分的选题背景和实践价值说明。
- Linux 内核模块设计说明。
- `/proc/oslab_monitor/overview`、`tasks`、`pid` 接口说明。
- `oslabctl` 用户态工具说明。
- 扩展部分运行截图和结果分析。
- 遇到的问题与解决方案。
- 项目总结。
- 参考资料。

报告中的代码 URL 必须清晰可访问，不能只写仓库名称。

## 14. 里程碑计划

### M1：基础框架完成

完成条件：

- 创建 `basic/`、`extension/`、`docs/` 目录。
- 为每个基础模块创建目录和 `Makefile`。
- 为扩展部分创建 `kernel/`、`user/`、`scripts/`、`tests/` 目录。

### M2：基础必做部分完成

完成条件：

- 调度模块实现 FCFS、SJF、RR、优先级调度。
- 内存模块实现 FF、BF、FIFO、LRU。
- 同步模块实现生产者-消费者、读者-写者、哲学家进餐。
- 文件系统模块实现创建、读写、删除、目录和空闲块管理。
- 每个模块有可运行示例。

### M3：扩展部分完成

完成条件：

- `oslab_monitor.ko` 可以编译、加载、卸载。
- `/proc/oslab_monitor/overview` 可读。
- `/proc/oslab_monitor/tasks` 可读。
- `/proc/oslab_monitor/pid` 可写入和读取。
- `oslabctl` 支持 `overview`、`tasks`、`pid <PID>`、`--help`。

### M4：测试与报告材料完成

完成条件：

- 基础部分四个模块均有运行截图或输出文本。
- 扩展部分有编译、加载、读取、卸载截图或输出文本。
- README 包含完整运行说明。
- 报告包含代码 URL 和访问方式。

## 15. 最终验收清单

### 15.1 基础部分验收清单

- [ ] `basic/scheduler/` 存在。
- [ ] 调度模块支持 FCFS、SJF、RR、优先级调度。
- [ ] 调度模块输出执行顺序和统计指标。
- [ ] `basic/memory/` 存在。
- [ ] 内存模块支持 FF、BF、FIFO、LRU。
- [ ] 内存模块输出分区状态、页框状态、缺页次数和缺页率。
- [ ] `basic/sync/` 存在。
- [ ] 同步模块支持生产者-消费者、读者-写者、哲学家进餐。
- [ ] 同步模块使用线程和同步机制，运行无死锁。
- [ ] `basic/filesystem/` 存在。
- [ ] 文件系统模块支持文件创建、读写、删除、目录和空闲块管理。

### 15.2 扩展部分验收清单

- [ ] `extension/oslab_monitor/kernel/oslab_monitor.c` 存在。
- [ ] `extension/oslab_monitor/kernel/Makefile` 存在。
- [ ] `make` 能生成 `oslab_monitor.ko`。
- [ ] `sudo insmod oslab_monitor.ko` 能加载模块。
- [ ] `/proc/oslab_monitor/overview` 存在且可读。
- [ ] `/proc/oslab_monitor/tasks` 存在且可读。
- [ ] `/proc/oslab_monitor/pid` 存在且支持写入 PID。
- [ ] `extension/oslab_monitor/user/oslabctl.c` 存在。
- [ ] `oslabctl overview` 可运行。
- [ ] `oslabctl tasks` 可运行。
- [ ] `oslabctl pid <PID>` 可运行。
- [ ] `sudo rmmod oslab_monitor` 能卸载模块并清理 `/proc` 节点。

### 15.3 文档与提交验收清单

- [ ] `docs/PRD.md` 存在。
- [ ] 根目录 `README.md` 存在。
- [ ] README 包含运行环境、编译方式、运行方式。
- [ ] README 包含扩展部分内核模块加载和卸载说明。
- [ ] 报告包含 GitHub URL 或类似平台 URL。
- [ ] 报告包含代码访问方式说明。
- [ ] 报告包含基础部分实验结果。
- [ ] 报告包含扩展部分实验结果。
- [ ] 报告包含问题分析和总结。

## 16. 开放问题

当前 PRD 已按以下决策固定范围：

- 基础部分完整覆盖四大模块。
- 基础部分算法组合采用 FCFS、SJF、RR、优先级调度、FF、BF、FIFO、LRU，以及三个经典同步问题。
- 技术栈采用 C + Linux CLI。
- 项目目录采用 `basic/` 和 `extension/` 分离结构。
- 扩展部分采用 `/proc/oslab_monitor/overview`、`tasks`、`pid` 和 `oslabctl`。
- 测试标准采用课程报告友好的手动测试、截图和关键输出。
- 输出文档路径为 `docs/PRD.md`。

后续若时间充足，可再决定是否增加以下增强项：

- 是否为基础部分增加统一 CLI 菜单。
- 是否为测试输出增加 CSV 文件。
- 是否为扩展部分增加 benchmark。
- 是否将报告草稿也放入 `docs/`。
