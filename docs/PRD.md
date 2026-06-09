# PRD：OSLab TraceBench 核心系统

## 1. 目标

OSLab TraceBench 提供确定性的用户态操作系统机制模型和 Linux 内核运行态观测扩展。核心系统强调命令行可复现执行、稳定文本输出和自动化验证脚本集成。

核心系统包含：

- 处理机调度模型。
- 内存管理模型。
- 同步工作负载。
- 内存型文件系统模型。
- Linux `/proc` 观测模块。
- `oslabctl` 用户态封装工具。

## 2. 目标用户

- 需要使用固定输入研究操作系统机制的研究者。
- 需要通过 Linux 内核模块验证进程和内存观测能力的开发者。
- 需要从干净检出复现模型输出和观测输出的审阅者。

## 3. 范围

包含：

- `basic/` 下四个独立用户态 C 模块。
- `extension/oslab_monitor/kernel/` 下一个 Linux 内核模块。
- `extension/oslab_monitor/user/` 下一个用户态 CLI。
- Bash 验证脚本和确定性输入样例。

不包含：

- Linux 内核源码修改。
- 宿主机调度器替换。
- 生产级文件系统实现。
- GUI 或 Web 前端。
- Windows 原生内核模块工作流。
- 将 WSL2 作为内核模块参考验证环境。

## 4. 运行环境

用户态模块：

```text
Linux
gcc
make
bash
pthread
```

内核模块：

```text
Ubuntu 22.04 LTS 或 Ubuntu 24.04 LTS VM
build-essential
linux-headers-$(uname -r)
insmod
rmmod
sudo/root 模块加载权限
```

## 5. 仓库结构

```text
basic/
├── scheduler/
├── memory/
├── sync/
└── filesystem/
extension/
└── oslab_monitor/
    ├── kernel/
    ├── user/
    ├── scripts/
    └── tests/
tests/
docs/
```

每个模块拥有自己的源码、头文件、构建文件、确定性输入和验证脚本。

## 6. 全局需求

- 所有用户态程序使用 C 实现。
- 每个模块支持 `make` 构建和 `make clean` 清理。
- 正常完成返回退出码 `0`。
- 参数非法或输入格式错误返回非零退出码。
- 错误信息以 `error:` 开头。
- 输出字段保持稳定，便于自动解析。
- 适用场景下平均值和比率保留两位小数。
- 模块之间不产生隐式依赖，除非文档明确说明。

## 7. 处理机调度

路径：`basic/scheduler/`

可执行文件：`scheduler`

命令：

```bash
./scheduler --algorithm fcfs|sjf|rr|priority < tests/sample.txt
```

需求：

- 解析进程数量、时间片、进程名、到达时间、服务时间和优先级。
- 实现 FCFS。
- 实现非抢占式 SJF。
- 实现 RR。
- 实现非抢占式优先级调度。
- 输出稳定时间线。
- 输出单进程开始时间、完成时间、等待时间、周转时间和带权周转时间。
- 输出平均等待时间、平均周转时间和平均带权周转时间。
- 在没有可运行进程时输出 `IDLE` 时间段。

输入规则：

- `process_count > 0`。
- RR 需要 `time_quantum > 0`。
- `arrival >= 0`。
- `burst > 0`。
- 优先级数值越低，调度优先级越高。

## 8. 内存管理

路径：`basic/memory/`

可执行文件：`memory`

命令：

```bash
./memory --mode partition --algorithm ff|bf < tests/partition.txt
./memory --mode paging --algorithm fifo|lru < tests/pages.txt
```

动态分区需求：

- 解析总内存大小。
- 分配命名内存块。
- 释放命名内存块。
- 实现首次适应。
- 实现最佳适应。
- 分配时拆分空闲区。
- 释放时合并相邻空闲区。
- 按地址升序输出已分配和空闲分区表。

页面置换需求：

- 解析页框数量和访问序列。
- 实现 FIFO 页面置换。
- 实现 LRU 页面置换。
- 每次访问后输出页框状态。
- 输出缺页次数和缺页率。

## 9. 进程同步

路径：`basic/sync/`

可执行文件：`sync`

命令：

```bash
./sync --problem producer_consumer --producers 2 --consumers 2 --buffer-size 4 --count 10
./sync --problem readers_writers --readers 3 --writers 2 --count 5
./sync --problem dining_philosophers --count 3
```

需求：

- 使用 POSIX pthread。
- 使用互斥锁、条件变量或等价同步原语。
- 实现有界缓冲区生产者-消费者。
- 实现写者优先读者-写者。
- 实现带死锁规避的哲学家进餐。
- 输出线程动作和最终汇总计数。
- 有限工作负载必须能结束且不死锁。

## 10. 内存型文件系统

路径：`basic/filesystem/`

可执行文件：`filesystem`

命令：

```bash
./filesystem < tests/fs_commands.txt
```

必需操作：

- `mkfs DISK_SIZE BLOCK_SIZE`
- `mkdir PATH`
- `create PATH`
- `write PATH CONTENT`
- `read PATH`
- `ls PATH`
- `delete PATH`
- `stat`

需求：

- 使用内存型虚拟磁盘。
- 使用固定大小块。
- 使用位图跟踪空闲块。
- 支持多级目录。
- 文件记录块列表。
- `write` 为覆盖写。
- `delete` 释放全部块。
- 如果写入容量不足，必须保留旧文件内容。

## 11. Linux 观测模块

路径：`extension/oslab_monitor/`

内核模块：

```text
extension/oslab_monitor/kernel/oslab_monitor.c
```

用户态 CLI：

```text
extension/oslab_monitor/user/oslabctl.c
```

proc 节点：

```text
/proc/oslab_monitor/overview
/proc/oslab_monitor/tasks
/proc/oslab_monitor/pid
```

权限：

| 节点 | 权限 |
|---|---:|
| `overview` | `0444` |
| `tasks` | `0444` |
| `pid` | `0644` |

`overview` 需求：

- 模块名。
- 内核版本。
- 总进程数。
- running、sleeping、stopped 和 zombie 进程数。
- 总内存、空闲内存和可用内存。
- 读取时的 jiffies。

`tasks` 需求：

- 稳定表头。
- PID。
- 进程命令名。
- 进程状态。
- 调度策略。
- 优先级。
- nice 值。
- 线程数。
- RSS。
- minor fault 和 major fault 计数。

`pid` 需求：

- 支持写入正整数 PID。
- 输出当前目标 PID 的详情。
- 目标 PID 不存在时输出明确状态。
- 对用户态写入长度做边界检查。

`oslabctl` 需求：

- `oslabctl --help`
- `oslabctl overview`
- `oslabctl tasks`
- `sudo oslabctl pid <PID>`
- 输出格式与对应 proc 节点保持一致。

## 12. 验证要求

用户态验证：

```bash
bash tests/run_all.sh
```

Ubuntu VM 内核模块验证：

```bash
cd extension/oslab_monitor
bash tests/test_oslab_monitor.sh
```

验证内容：

- 所有基础模块能构建。
- 所有基础模块固定输入输出稳定。
- 错误输入返回非零并输出 `error:`。
- 同步模块默认工作负载可在有限时间内结束。
- 内核模块能构建、加载、读取、写入目标 PID 并卸载。
- 卸载后 `/proc/oslab_monitor/` 被清理。

## 13. 完成标准

- [ ] `tests/run_all.sh` 验证全部用户态模块。
- [ ] 每个基础模块支持 `--help` 或明确用法输出。
- [ ] `overview` 包含所有必需字段。
- [ ] `tasks` 包含稳定表头。
- [ ] `pid` 包含所有必需字段。
- [ ] `oslabctl` 与 proc 输出格式一致。
- [ ] Ubuntu VM 中内核模块验证通过。
