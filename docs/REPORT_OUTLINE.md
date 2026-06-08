# 课程报告提纲

本文档用于整理 OSLab TraceBench 课程设计最终报告。报告正文可按以下章节展开，并将命令输出或截图补充到对应位置。

## 1. 项目基本信息

- 项目名称：OSLab TraceBench 操作系统课程设计。
- 小组成员及贡献说明：若为个人完成，说明个人完成；若为小组完成，列出每位成员负责模块。
- 代码 URL：填写 GitHub 或类似平台仓库地址。
- 代码访问方式：说明分支、克隆命令和必要权限。

## 2. 运行环境

- 基础部分环境：Linux CLI、`gcc`、`make`、`pthread`、`bash`。
- 扩展部分环境：Ubuntu 22.04 LTS 或 Ubuntu 24.04 LTS VM 默认内核。
- 内核模块依赖：`build-essential`、`linux-headers-$(uname -r)`、`kbuild`。
- 记录实际验证环境：发行版、内核版本、gcc 版本、make 版本。

## 3. 项目目录结构

- 说明 `basic/` 与 `extension/` 的分层。
- 说明四个基础模块互相独立。
- 说明扩展模块中 `kernel/`、`user/`、`scripts/`、`tests/` 的职责。

## 4. 基础必做部分设计与实现

### 4.1 处理机调度

- 模块路径：`basic/scheduler/`。
- 实现算法：FCFS、非抢占式 SJF、RR、非抢占式优先级调度。
- 核心数据结构：`Process`、`Timeline`、`Segment`。
- 重点说明：平局规则、`IDLE` 时间段、等待时间/周转时间/带权周转时间计算。
- 运行命令：`cd basic/scheduler && bash tests/run_tests.sh`。
- 补充运行截图或输出文本。

### 4.2 内存管理

- 模块路径：`basic/memory/`。
- 实现算法：动态分区 FF/BF、页面置换 FIFO/LRU。
- 核心数据结构：分区链表、页框数组、装入时间、最近访问时间。
- 重点说明：分区拆分、回收合并、缺页次数和缺页率。
- 运行命令：`cd basic/memory && bash tests/run_tests.sh`。
- 补充运行截图或输出文本。

### 4.3 进程同步与并发控制

- 模块路径：`basic/sync/`。
- 实现问题：生产者-消费者、写者优先读者-写者、哲学家进餐。
- 核心机制：`pthread_mutex_t`、`pthread_cond_t`、最多 4 个哲学家同时竞争筷子。
- 重点说明：线程退出条件、无死锁验证、汇总计数。
- 运行命令：`cd basic/sync && bash tests/run_tests.sh`。
- 补充运行截图或输出文本。

### 4.4 文件系统

- 模块路径：`basic/filesystem/`。
- 实现功能：`mkfs`、`mkdir`、`create`、`write`、`read`、`ls`、`delete`、`stat`。
- 核心数据结构：内存目录树、块位图、文件块列表。
- 重点说明：多级目录、覆盖写、删除释放块、空间不足时保持旧内容。
- 运行命令：`cd basic/filesystem && bash tests/run_tests.sh`。
- 补充运行截图或输出文本。

## 5. 基础部分统一测试

- 命令：`bash tests/run_all.sh`。
- 说明根测试只运行基础四模块。
- 补充根测试输出文本或截图。

## 6. 自由扩展提升部分设计与实现

### 6.1 选题背景和实践价值

- 选题方向：Linux 内核与系统编程。
- 实践价值：通过 `/proc` 将课程中的进程、调度、内存概念连接到真实 Linux 运行态。

### 6.2 Linux 内核模块

- 模块路径：`extension/oslab_monitor/kernel/`。
- 模块文件：`oslab_monitor.c`。
- 构建文件：`Makefile`。
- 生命周期：`module_init` 创建 `/proc/oslab_monitor/`，`module_exit` 清理全部节点。
- 输出机制：`seq_file`。

### 6.3 `/proc/oslab_monitor/overview`

- 权限：`0444`。
- 字段：`module:`、`kernel:`、`total_tasks:`、`running_tasks:`、`sleeping_tasks:`、`stopped_tasks:`、`zombie_tasks:`、`mem_total_kb:`、`mem_free_kb:`、`mem_available_kb:`、`read_time_jiffies:`。
- 补充读取截图或输出文本。

### 6.4 `/proc/oslab_monitor/tasks`

- 权限：`0444`。
- 字段表头：`PID COMM STATE POLICY PRIO NICE THREADS RSS_KB MIN_FLT MAJ_FLT`。
- 重点说明：遍历全部进程、`mm == NULL` 时 RSS 输出 `0`、缺页字段保留为 `N/A`。
- 补充读取截图或输出文本。

### 6.5 `/proc/oslab_monitor/pid`

- 权限：`0644`。
- 行为：写入目标 PID 后读取详情；PID 不存在时输出 `not found`。
- 安全处理：限制输入长度、`copy_from_user()`、`kstrtoint()`。
- 补充写入 PID 1 和读取结果截图或输出文本。

### 6.6 用户态工具 `oslabctl`

- 路径：`extension/oslab_monitor/user/oslabctl.c`。
- 命令：`--help`、`overview`、`tasks`、`pid <PID>`。
- 设计原则：原样输出 `/proc` 内容。
- 补充运行截图或输出文本。

### 6.7 扩展脚本和集成测试

- 加载脚本：`extension/oslab_monitor/scripts/load.sh`。
- 演示脚本：`extension/oslab_monitor/scripts/demo.sh`。
- 卸载脚本：`extension/oslab_monitor/scripts/unload.sh`。
- 集成测试：`extension/oslab_monitor/tests/test_oslab_monitor.sh`。
- 运行命令：`cd extension/oslab_monitor && bash tests/test_oslab_monitor.sh`。
- 说明该测试必须在 Ubuntu VM 中运行。

## 7. 遇到的问题与解决方案

- 基础模块算法平局规则容易导致 oracle 不一致：统一按 PRD 中到达时间和输入顺序处理。
- RR 入队顺序容易出错：使用固定样例测试执行序列。
- 并发程序可能无法退出：使用明确目标计数和 `timeout 5s` 自动测试。
- WSL2 不是扩展验收环境：内核模块编译加载需要 Ubuntu VM 和当前内核 headers。

## 8. 项目总结

- 总结基础四模块实现效果。
- 总结扩展部分与真实 Linux 系统的联系。
- 总结测试、文档和后续改进方向。

## 9. 参考资料

- Linux Kernel Documentation。
- `proc_fs` 和 `seq_file` 相关资料。
- POSIX pthread 文档。
- 操作系统课程教材和课堂资料。
