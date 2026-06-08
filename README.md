# OS-Design

《操作系统》课程设计项目文档与实现仓库。

本项目采用“基础必做部分 + 自由扩展提升部分”的两级结构：

- 基础必做部分：使用 C 语言命令行程序模拟处理机调度、内存管理、进程同步与并发控制、文件系统。
- 自由扩展提升部分：使用 Linux 内核模块和用户态 CLI 工具实现基于 `/proc` 的进程与内存运行态观测系统。

当前仓库处于课程设计实现阶段，已完成：

- 基础四模块：`scheduler`、`memory`、`sync`、`filesystem`。
- 基础四模块 Bash 测试入口：`tests/run_all.sh`。
- 扩展模块源码：`oslab_monitor.ko`、`/proc/oslab_monitor/`、`oslabctl`、加载/卸载/演示脚本和集成测试脚本。
- 项目文档：`docs/PRD.md`、`docs/TECH.md`、`docs/FEATURES.md`、`docs/PLAN.md`、`docs/REPORT_OUTLINE.md`、`docs/DOC_AUDIT.md`。

验证状态：

- 基础四模块已在 Ubuntu 24.04.2 LTS VM 中通过 `bash tests/run_all.sh`。
- 扩展模块已在 Ubuntu 24.04.2 LTS VM 中通过 `cd extension/oslab_monitor && bash tests/test_oslab_monitor.sh`，覆盖内核模块编译、加载、`/proc` 读取、`oslabctl` 和卸载清理。
- WSL2 仍不作为扩展部分默认验收环境；内核模块最终验收以安装当前内核 headers 的 Ubuntu VM 为准。

## 项目结构

目标结构如下：

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
│   ├── PLAN.md
│   ├── REPORT_OUTLINE.md
│   ├── DOC_AUDIT.md
│   └── prompts/
├── tests/
│   └── run_all.sh
└── README.md
```

## 文档

- [PRD.md](docs/PRD.md)：定义项目目标、范围、功能需求、验收标准、测试要求和报告要求。
- [TECH.md](docs/TECH.md)：定义模块架构、核心数据结构、算法设计、内核接口设计、测试策略和架构决策。
- [FEATURES.md](docs/FEATURES.md)：汇总用户可见功能、目标命令和非目标。
- [PLAN.md](docs/PLAN.md)：实现计划、任务顺序、文件清单和测试计划。
- [REPORT_OUTLINE.md](docs/REPORT_OUTLINE.md)：课程报告提纲。
- [DOC_AUDIT.md](docs/DOC_AUDIT.md)：文档与代码一致性审计。
- [design.md](docs/prompts/design.md)：课程设计原始要求摘录，用于追溯需求来源。

文档关系：

- `PRD.md` 回答“要做什么”和“如何验收”。
- `TECH.md` 回答“如何实现”和“为什么这样设计”。
- `FEATURES.md` 回答“最终对用户暴露哪些功能”。
- `PLAN.md` 回答“按什么顺序实现和验收”。
- `README.md` 作为仓库入口，说明结构、环境和运行方式。

## 运行环境

基础部分目标环境：

- Linux 命令行环境。
- `gcc`
- `make`
- `pthread`
- `bash`

扩展部分目标环境：

- Ubuntu 22.04 LTS 或 Ubuntu 24.04 LTS 虚拟机默认内核。
- `build-essential`
- `linux-headers-$(uname -r)`
- `kbuild`
- `insmod`
- `rmmod`
- `lsmod`
- `dmesg`

扩展部分不以 WSL2 作为默认验收环境。

## 基础部分目标命令

一键运行基础四模块测试：

```bash
bash tests/run_all.sh
```

调度模块：

```bash
cd basic/scheduler
make
./scheduler --algorithm fcfs < tests/sample.txt
./scheduler --algorithm sjf < tests/sample.txt
./scheduler --algorithm rr < tests/sample.txt
./scheduler --algorithm priority < tests/sample.txt
bash tests/run_tests.sh
make clean
```

内存模块：

```bash
cd basic/memory
make
./memory --mode partition --algorithm ff < tests/partition.txt
./memory --mode partition --algorithm bf < tests/partition.txt
./memory --mode paging --algorithm fifo < tests/pages.txt
./memory --mode paging --algorithm lru < tests/pages.txt
bash tests/run_tests.sh
make clean
```

同步模块：

```bash
cd basic/sync
make
./sync --problem producer_consumer --producers 2 --consumers 2 --buffer-size 4 --count 10
./sync --problem readers_writers --readers 3 --writers 2 --count 5
./sync --problem dining_philosophers --count 3
bash tests/run_tests.sh
make clean
```

文件系统模块：

```bash
cd basic/filesystem
make
./filesystem < tests/fs_commands.txt
bash tests/run_tests.sh
make clean
```

## 扩展部分目标命令

内核模块：

```bash
cd extension/oslab_monitor/kernel
make
sudo insmod oslab_monitor.ko
ls /proc/oslab_monitor
cat /proc/oslab_monitor/overview
cat /proc/oslab_monitor/tasks
echo 1 | sudo tee /proc/oslab_monitor/pid
cat /proc/oslab_monitor/pid
sudo rmmod oslab_monitor
dmesg | tail
make clean
```

用户态工具：

```bash
cd extension/oslab_monitor/user
make
./oslabctl --help
./oslabctl overview
./oslabctl tasks
sudo ./oslabctl pid 1
make clean
```

脚本：

```bash
cd extension/oslab_monitor
bash scripts/load.sh
bash scripts/demo.sh
bash scripts/unload.sh
bash tests/test_oslab_monitor.sh
```

注意：扩展部分脚本会调用 `sudo insmod`、`sudo rmmod` 和 `/proc/oslab_monitor/`，必须在安装了当前内核 headers 的 Ubuntu 22.04/24.04 VM 中运行。WSL2 默认不是扩展部分验收环境。

## 验收重点

基础部分：

- 四个模块均可独立编译、运行、清理。
- 调度模块输出固定样例的预期执行序列和统计结果。
- 内存模块输出动态分区过程、页面置换过程、缺页次数和缺页率。
- 同步模块默认参数下自动结束，无死锁。
- 文件系统支持 `mkfs`、`mkdir`、`create`、`write`、`read`、`ls`、`delete`、`stat`。

扩展部分：

- `oslab_monitor.ko` 可编译、加载、卸载。
- `/proc/oslab_monitor/overview`、`tasks`、`pid` 存在且字段稳定。
- `oslabctl overview`、`oslabctl tasks`、`sudo oslabctl pid <PID>` 可运行。
- 卸载后 `/proc/oslab_monitor/` 被清理。

报告材料：

- GitHub 或类似平台代码 URL。
- 代码访问方式。
- Linux 发行版、内核版本、编译器版本。
- 基础四模块运行截图或输出文本。
- 扩展部分编译、加载、读取 `/proc`、卸载截图或输出文本。
- 问题分析和项目总结。
