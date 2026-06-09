# OSLab TraceBench

操作系统课程设计项目。当前已完成基础 OS 机制模拟、Linux `/proc` 内核观测扩展，以及 TraceBench v2 P0 资源压力与系统观测实验平台。

基础部分用 C 命令行程序模拟：

- 处理机调度：FCFS、SJF、RR、非抢占式优先级调度。
- 内存管理：动态分区 FF/BF、页面置换 FIFO/LRU。
- 进程同步：生产者-消费者、读者-写者、哲学家进餐。
- 文件系统：内存型虚拟磁盘、多级目录、块位图、文件读写删除。

扩展部分实现 Linux 内核模块 `oslab_monitor.ko`，通过 `/proc/oslab_monitor/` 输出系统概览、进程列表和指定 PID 信息，并提供用户态工具 `oslabctl`。

## 实际使用场景

本项目面向操作系统课程设计验收和实验报告整理，典型使用场景如下：

- 课程验收人员拉取仓库后，先运行 `bash tests/run_all.sh`，快速确认调度、内存、同步和文件系统四个基础模块都能编译、运行并通过固定样例测试。
- 学生编写基础实验报告时，分别运行 `scheduler`、`memory`、`sync`、`filesystem`，截取算法执行序列、分区或页框变化、线程同步日志和虚拟文件系统状态，用于解释 OS 基础机制。
- 学生演示 Linux 内核与系统编程扩展时，在 Ubuntu VM 中加载 `oslab_monitor.ko`，读取 `/proc/oslab_monitor/overview`、`tasks`、`pid`，把真实 Linux 系统中的进程状态、调度字段和内存字段与课堂概念对应起来。
- 学生或助教需要简化 `/proc` 操作时，使用 `oslabctl overview`、`oslabctl tasks`、`sudo oslabctl pid 1` 获取与直接 `cat /proc/oslab_monitor/*` 一致的输出。
- 学生做 TraceBench v2 资源压力实验时，在 Ubuntu VM 中运行 CPU、memory、io 三类 profile，采集 PSI、cgroup v2 和可选 `oslab_monitor` 对照指标，生成 `samples.csv`、`summary.txt` 和 Markdown 报告。
- 课程提交前，使用 README 中的基础测试、扩展测试和 TraceBench P0 测试命令复现实验结果，确认报告中的环境、命令和输出材料可重新生成。

## TraceBench v2

TraceBench v2 是新增扩展模块，不替代基础四模块和 `extension/oslab_monitor/`。P0 已实现 `extension/tracebench/tracebench`，可在 Ubuntu VM 中运行 CPU、memory、io 压力实验，采集 PSI、cgroup v2 和可选 `/proc/oslab_monitor/overview` 对照指标，输出 `samples.csv`、`summary.txt` 和 Markdown 报告。

v2 配套文档：

- `docs/PRDv2.md`：定义 TraceBench v2 的 P0/P1/P2 需求、命令、输出和验收标准。
- `docs/TECHv2.md`：定义 P0 技术方案、模块边界、数据结构、CSV 字段、权限模型和测试策略。
- `docs/PLANv2.md`：定义代码文件、脚本、测试文件和验证顺序。
- `docs/TRACEBENCH_REPORT_TEMPLATE.md`：定义 v2 实验报告整理结构。

P0 环境要求：

```text
Ubuntu 24.04 LTS VM
cgroup v2
/proc/pressure/cpu
/proc/pressure/memory
/proc/pressure/io
gcc
make
sudo/root
```

构建和帮助：

```bash
cd extension/tracebench
make
./tracebench --help
```

运行三类 profile：

```bash
sudo ./tracebench run --profile cpu --duration 3 --sample-interval 1 --cpu-workers 2 --output output/test_cpu
sudo ./tracebench run --profile memory --duration 3 --sample-interval 1 --memory-mb 64 --output output/test_memory
sudo ./tracebench run --profile io --duration 3 --sample-interval 1 --io-mb 16 --output output/test_io
```

生成报告和清理：

```bash
./tracebench report --input output/test_cpu --output output/test_cpu/report.md
sudo ./tracebench cleanup
```

P0 集成测试：

```bash
sudo bash tests/test_tracebench.sh
```

## 环境

基础部分：

```bash
gcc
make
bash
pthread
```

扩展部分：

```bash
Ubuntu 22.04/24.04 VM
build-essential
linux-headers-$(uname -r)
insmod
rmmod
```

说明：扩展部分需要能加载内核模块的 Ubuntu VM，不以 WSL2 作为验收环境。

## 一键测试基础模块

```bash
bash tests/run_all.sh
```

该命令会依次测试：

```text
basic/scheduler
basic/memory
basic/filesystem
basic/sync
```

## 单独运行基础模块

调度：

```bash
cd basic/scheduler
make
./scheduler --algorithm fcfs < tests/sample.txt
./scheduler --algorithm sjf < tests/sample.txt
./scheduler --algorithm rr < tests/sample.txt
./scheduler --algorithm priority < tests/sample.txt
make clean
```

内存：

```bash
cd basic/memory
make
./memory --mode partition --algorithm ff < tests/partition.txt
./memory --mode partition --algorithm bf < tests/partition.txt
./memory --mode paging --algorithm fifo < tests/pages.txt
./memory --mode paging --algorithm lru < tests/pages.txt
make clean
```

同步：

```bash
cd basic/sync
make
./sync --problem producer_consumer --producers 2 --consumers 2 --buffer-size 4 --count 10
./sync --problem readers_writers --readers 3 --writers 2 --count 5
./sync --problem dining_philosophers --count 3
make clean
```

文件系统：

```bash
cd basic/filesystem
make
./filesystem < tests/fs_commands.txt
make clean
```

## 运行扩展模块

在 Ubuntu VM 中执行：

```bash
cd extension/oslab_monitor
bash tests/test_oslab_monitor.sh
```

手动演示：

```bash
cd extension/oslab_monitor/kernel
make
sudo insmod oslab_monitor.ko
cat /proc/oslab_monitor/overview
cat /proc/oslab_monitor/tasks
echo 1 | sudo tee /proc/oslab_monitor/pid
cat /proc/oslab_monitor/pid
sudo rmmod oslab_monitor
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

## 项目结构

```text
basic/                       基础四个独立 CLI 模块
extension/oslab_monitor/      Linux 内核模块、oslabctl、脚本和集成测试
extension/tracebench/         TraceBench v2 用户态压力实验工具
tests/run_all.sh              基础模块一键测试入口
docs/COURSE_REPORT.md         课程设计报告
docs/PRD.md                   需求文档
docs/PRDv2.md                 TraceBench v2 需求文档
docs/TECH.md                  技术方案
docs/TECHv2.md                TraceBench v2 技术方案
docs/FEATURES.md              功能清单
docs/PLAN.md                  实现计划
docs/PLANv2.md                TraceBench v2 实现计划
docs/TRACEBENCH_REPORT_TEMPLATE.md TraceBench v2 报告模板
docs/DOC_AUDIT.md             文档一致性审计
```

## 已验证环境

```text
Ubuntu 24.04.2 LTS
Linux 6.11.0-17-generic
gcc 13.3.0
GNU Make 4.3
```

已通过：

```bash
bash tests/run_all.sh
cd extension/oslab_monitor
bash tests/test_oslab_monitor.sh
cd extension/tracebench
sudo bash tests/test_tracebench.sh
```
