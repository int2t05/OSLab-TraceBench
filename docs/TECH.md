# TECH：核心系统技术设计

## 1. 架构

核心系统分为两层：

- `basic/` 下的用户态 OS 机制模型。
- `extension/oslab_monitor/` 下的 Linux 运行态观测模块。

用户态模型是相互独立的 C 命令行程序。它们不共享公共库，可以单独构建、执行和验证。

观测扩展是一个 Linux 内核模块，通过 `/proc/oslab_monitor/` 导出进程和内存状态；用户态 CLI 只负责转发 proc 输出。

## 2. 构建模型

每个模块拥有自己的 `Makefile`，支持：

```bash
make
make clean
```

用户态通用编译要求：

```text
gcc
-Wall
-Wextra
C11 兼容代码
```

同步模块链接 pthread。内核模块通过宿主内核构建系统和本机 `linux-headers-$(uname -r)` 构建。

## 3. 用户态模块布局

每个基础模块遵循：

```text
basic/<module>/
├── Makefile
├── include/
│   └── <module>.h
├── src/
│   ├── main.c
│   ├── parser.c
│   ├── <module>.c
│   └── output.c
└── tests/
```

职责：

- `main.c`：CLI 分发和顶层错误处理。
- `parser.c`：文本输入解析和校验。
- 核心实现文件：算法或机制逻辑。
- `output.c`：稳定输出格式。
- `include/*.h`：模块内部共享类型和函数声明。

## 4. 调度模块设计

路径：`basic/scheduler/`

核心结构：

- `Process`：进程名、到达时间、服务时间、优先级、剩余时间、开始/完成时间和输入顺序。
- `Segment`：时间线段名和时间区间。
- `Timeline`：动态时间线数组。

算法：

- FCFS 按到达时间和输入顺序调度。
- SJF 为非抢占式，从已到达且未完成进程中选择服务时间最短者。
- Priority 为非抢占式，数值更低表示优先级更高。
- RR 使用就绪队列和时间片推进。

当 CPU 没有可运行进程时，调度器记录 `IDLE` 时间线段。

统计公式：

```text
turnaround = finish - arrival
waiting = turnaround - burst
weighted_turnaround = turnaround / burst
```

## 5. 内存模块设计

路径：`basic/memory/`

动态分区模式：

- 使用链表表示分区。
- 支持首次适应和最佳适应搜索策略。
- 分配时拆分分区。
- 释放时合并相邻空闲分区。
- 按起始地址升序输出已分配和空闲表。

页面置换模式：

- 使用数组表示页框。
- FIFO 使用装入时间戳。
- LRU 使用最近访问时间戳。
- 空页框用 `-1` 表示。
- 缺页率计算为 `faults / accesses * 100`。

## 6. 同步模块设计

路径：`basic/sync/`

生产者-消费者：

- 互斥锁。
- `not_full` 条件变量。
- `not_empty` 条件变量。
- 环形缓冲区。
- 有限生产目标和有限全局消费目标。

读者-写者：

- 写者优先策略。
- 活跃读者/写者计数。
- 等待写者计数。
- 读条件变量和写条件变量。

哲学家进餐：

- 五个哲学家。
- 五个筷子互斥锁。
- 房间限制器最多允许四个哲学家同时竞争筷子。

## 7. 文件系统设计

路径：`basic/filesystem/`

文件系统是内存模型：

- 目录树根为 `/`。
- 文件节点记录大小和块列表。
- 目录节点记录子节点。
- 位图记录块分配状态。
- 块数据存放在内存中。

写入采用覆盖语义。替换文件内容前会先检查空闲块是否足够，因此失败写入不会破坏旧文件内容。

路径规则：

- 仅支持绝对路径。
- 限制最大路径长度。
- 限制组件名最大长度。
- 组件名允许字母、数字、`_`、`-` 和 `.`。

## 8. Linux 观测模块设计

路径：`extension/oslab_monitor/`

内核模块：

```text
kernel/oslab_monitor.c
```

proc 树：

```text
/proc/oslab_monitor/
├── overview
├── tasks
└── pid
```

权限：

| 节点 | 模式 |
|---|---:|
| `overview` | `0444` |
| `tasks` | `0444` |
| `pid` | `0644` |

实现要点：

- proc 节点使用 `seq_file` 风格输出，保证长文本读取稳定。
- 进程遍历使用内核进程迭代接口。
- 内存信息通过安全内核接口读取。
- 对内核线程或无地址空间进程处理 `mm == NULL`。
- 模块退出时清理全部 proc 节点。

## 9. `overview`

`overview` 输出：

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

实现会扫描进程并读取内核内存信息。数值字段输出非负值。

## 10. `tasks`

`tasks` 输出稳定表格：

```text
PID     COMM            STATE   POLICY  PRIO  NICE  THREADS  RSS_KB  MIN_FLT  MAJ_FLT
```

实现遍历全部进程并格式化调度策略、优先级、nice 值、线程数、RSS 和缺页字段。缺失或不可用值不应破坏列布局。

## 11. `pid`

`pid` 支持：

- 写入正整数目标 PID。
- 读取选中 PID 的详情。
- 进程不存在时报告 not found。
- 对 `copy_from_user` 输入做长度边界处理。

读取字段：

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

## 12. `oslabctl`

路径：`extension/oslab_monitor/user/`

`oslabctl` 不重新格式化 proc 内容：

```bash
./oslabctl overview
./oslabctl tasks
sudo ./oslabctl pid 1
```

这样可以避免维护两套输出格式，并让 proc 接口成为唯一事实来源。

## 13. 验证

用户态验证：

```bash
bash tests/run_all.sh
```

内核模块验证：

```bash
cd extension/oslab_monitor
bash tests/test_oslab_monitor.sh
```

验证脚本检查退出码、必需字段、确定性输出和清理行为。内核模块脚本使用 trap，确保中途失败时尽量卸载模块。

## 14. 失败模式

| 失败模式 | 处理方式 |
|---|---|
| 输入格式错误 | 严格解析，返回非零，输出 `error:` |
| 调度平局不明确 | 使用已文档化的到达时间和输入顺序规则 |
| 释放后分区碎片 | 合并相邻空闲分区 |
| 文件系统写入失败 | 替换旧内容前先检查容量 |
| 同步死锁 | 有限计数和死锁规避策略 |
| 长进程列表截断 | 使用 `seq_file` 风格输出 |
| 进程没有 `mm` | 检查空地址空间指针 |
| proc PID 写入溢出 | 有界用户缓冲区和整数解析 |
| 卸载后残留 proc 节点 | 按创建逆序删除节点 |

## 15. 设计决策

### DD-001：四个独立用户态 CLI

不同 OS 机制的输入、输出和验证样例差异明显。独立二进制可以保持模块边界清晰，减少隐藏耦合。

### DD-002：不引入跨模块公共库

模块之间故意保留少量解析和输出辅助逻辑重复，避免共享行为耦合独立机制模型。

### DD-003：内存型文件系统

文件系统建模目录、元数据和块分配行为，但不持久化磁盘状态，从而保持机制聚焦且可复现。

### DD-004：使用 `/proc` 和 `seq_file` 做观测

`/proc` 可直接通过 shell 工具检查，适合进程和内存观测。`seq_file` 可以避免进程列表使用固定缓冲区导致截断。

### DD-005：`oslabctl` 原样转发

用户态封装只读写 proc 节点，不重新解释内核输出。这样可以把 proc 接口保持为单一事实来源。
