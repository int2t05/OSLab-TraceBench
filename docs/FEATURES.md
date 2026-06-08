# 功能清单

本文档汇总 OSLab TraceBench 项目的用户可见功能。详细需求以 `docs/PRD.md` 为准，技术实现以 `docs/TECH.md` 为准。

当前验证状态：

- 基础四模块已在 Ubuntu 24.04.2 LTS VM 中通过 `bash tests/run_all.sh`。
- 扩展部分已在 Ubuntu 24.04.2 LTS VM 中通过 `cd extension/oslab_monitor && bash tests/test_oslab_monitor.sh`，覆盖内核模块编译、加载、读取、`oslabctl` 和卸载清理。

## 1. 基础必做部分

基础必做部分位于 `basic/`，由四个独立命令行程序组成。

### 1.1 处理机调度

目录：`basic/scheduler/`

可执行文件：`scheduler`

功能：

- 支持 FCFS。
- 支持非抢占式 SJF。
- 支持 RR。
- 支持非抢占式优先级调度。
- 支持动态输入进程数量、到达时间、服务时间、优先级和 RR 时间片。
- 输出执行序列、每个进程统计表和平均指标。

目标命令：

```bash
./scheduler --algorithm fcfs < tests/sample.txt
./scheduler --algorithm sjf < tests/sample.txt
./scheduler --algorithm rr < tests/sample.txt
./scheduler --algorithm priority < tests/sample.txt
```

### 1.2 内存管理

目录：`basic/memory/`

可执行文件：`memory`

功能：

- 支持动态分区管理。
- 支持首次适应 FF。
- 支持最佳适应 BF。
- 支持内存分配、回收和相邻空闲分区合并。
- 支持页面置换。
- 支持 FIFO 页面置换。
- 支持 LRU 页面置换。
- 输出分区状态、页框状态、缺页次数和缺页率。

目标命令：

```bash
./memory --mode partition --algorithm ff < tests/partition.txt
./memory --mode partition --algorithm bf < tests/partition.txt
./memory --mode paging --algorithm fifo < tests/pages.txt
./memory --mode paging --algorithm lru < tests/pages.txt
```

### 1.3 进程同步与并发控制

目录：`basic/sync/`

可执行文件：`sync`

功能：

- 支持生产者-消费者问题。
- 支持读者-写者问题。
- 支持哲学家进餐问题。
- 使用 POSIX 线程和同步机制。
- 输出线程动作、等待、唤醒、资源状态和汇总统计。
- 默认参数下自动结束。

目标命令：

```bash
./sync --problem producer_consumer --producers 2 --consumers 2 --buffer-size 4 --count 10
./sync --problem readers_writers --readers 3 --writers 2 --count 5
./sync --problem dining_philosophers --count 3
```

### 1.4 文件系统

目录：`basic/filesystem/`

可执行文件：`filesystem`

功能：

- 支持内存型虚拟磁盘。
- 支持多级目录。
- 支持块位图空闲空间管理。
- 支持文件创建、覆盖写、读取和删除。
- 支持目录创建和目录列表展示。
- 支持文件系统整体状态输出。

目标命令：

```bash
./filesystem < tests/fs_commands.txt
```

支持命令：

- `mkfs DISK_SIZE BLOCK_SIZE`
- `mkdir PATH`
- `create PATH`
- `write PATH CONTENT`
- `read PATH`
- `ls PATH`
- `delete PATH`
- `stat`

## 2. 自由扩展提升部分

扩展部分位于 `extension/oslab_monitor/`，实现 Linux 内核与系统编程方向。

### 2.1 内核模块

目录：`extension/oslab_monitor/kernel/`

模块：`oslab_monitor.ko`

功能：

- 编译生成 Linux 内核模块。
- 加载时创建 `/proc/oslab_monitor/`。
- 卸载时清理所有 `/proc` 节点。
- 输出加载和卸载日志。

目标命令：

```bash
make
sudo insmod oslab_monitor.ko
sudo rmmod oslab_monitor
```

### 2.2 `/proc/oslab_monitor/overview`

功能：

- 输出模块名称。
- 输出内核版本。
- 输出总进程数量。
- 输出运行态、睡眠态、停止态、僵尸态进程数量。
- 输出总内存、空闲内存、可用内存。
- 输出读取时间字段。

目标命令：

```bash
cat /proc/oslab_monitor/overview
```

### 2.3 `/proc/oslab_monitor/tasks`

功能：

- 输出稳定表头。
- 遍历当前系统进程列表。
- 输出 PID、进程名、状态、调度策略、优先级、nice 值、线程数、RSS、缺页字段。

目标命令：

```bash
cat /proc/oslab_monitor/tasks
```

### 2.4 `/proc/oslab_monitor/pid`

功能：

- 支持写入目标 PID。
- 支持读取指定 PID 的进程详情。
- PID 不存在时输出明确提示。
- 非法 PID 不导致模块崩溃。

权限：

- `overview/tasks = 0444`
- `pid = 0644`

目标命令：

```bash
echo 1 | sudo tee /proc/oslab_monitor/pid
cat /proc/oslab_monitor/pid
```

### 2.5 用户态工具 `oslabctl`

目录：`extension/oslab_monitor/user/`

可执行文件：`oslabctl`

功能：

- `oslabctl --help`
- `oslabctl overview`
- `oslabctl tasks`
- `sudo oslabctl pid <PID>`
- 模块未加载时输出明确错误提示。
- 参数错误或权限不足时输出明确错误提示。

目标命令：

```bash
./oslabctl --help
./oslabctl overview
./oslabctl tasks
sudo ./oslabctl pid 1
```

## 3. 非目标

本项目不包含：

- GUI 图形界面。
- Web 服务或前端页面。
- 真实 Linux 调度器修改。
- Linux 内核源码修改。
- 生产级完整文件系统。
- WSL2 作为扩展部分默认验收环境。
