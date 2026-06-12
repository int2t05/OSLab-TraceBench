# OSLab TraceBench

[![许可证: MulanPSL-2.0](https://img.shields.io/badge/%E8%AE%B8%E5%8F%AF%E8%AF%81-MulanPSL--2.0-blue.svg)](LICENSE)
[![语言: C](https://img.shields.io/badge/%E8%AF%AD%E8%A8%80-C-555555.svg)](https://en.cppreference.com/w/c)
[![平台: Linux](https://img.shields.io/badge/%E5%B9%B3%E5%8F%B0-Linux-222222.svg)](https://www.kernel.org/)
[![构建: Make](https://img.shields.io/badge/%E6%9E%84%E5%BB%BA-Make-427819.svg)](https://www.gnu.org/software/make/)

OSLab TraceBench 是一个面向操作系统机制建模与 Linux 运行态观测的 C/Linux 研究型仓库。项目同时提供确定性的用户态 OS 机制模型、基于 `/proc` 的 Linux 内核观测模块，以及用于 CPU、内存和 I/O 压力采样的 TraceBench 工具，便于在 Ubuntu VM 中复现系统行为并生成可分析的数据材料。

## 项目范围

本仓库包含三个可独立使用的部分：

- `basic/`：处理机调度、内存管理、进程同步和内存型文件系统的 C 命令行模型。
- `extension/oslab_monitor/`：Linux 内核模块和用户态 CLI，通过 `/proc/oslab_monitor/` 导出进程和内存状态。
- `extension/tracebench/`：用户态资源压力与采样工具，将 PSI、cgroup v2 和可选 `oslab_monitor` 指标写入 CSV、摘要和 Markdown 报告。

本项目不修改 Linux 内核源码，不替换宿主机调度器，也不提供 GUI 或 Web 服务。所有内核相关能力均限制在可加载内核模块和 `/proc` 接口内。

## 实际使用场景

- 对比 FCFS、SJF、RR 和优先级调度在固定输入下的时间线和平均指标。
- 观察动态分区分配、回收、合并以及 FIFO/LRU 页面置换过程。
- 运行生产者-消费者、读者-写者和哲学家进餐并发模型，检查同步行为和终止性。
- 在 Ubuntu VM 中加载 `oslab_monitor.ko`，直接读取真实 Linux 进程、调度和内存字段。
- 使用 TraceBench 生成 CPU、内存和 I/O 压力，同时采样 PSI、cgroup v2 和可选内核观测指标。
- 生成 `samples.csv`、`summary.txt` 和 Markdown 运行报告，用于系统行为分析和复现实验数据。

## 环境要求

基础用户态模块：

```text
Linux
gcc
make
bash
pthread
```

内核模块和 TraceBench 完整验证：

```text
Ubuntu 22.04 LTS 或 Ubuntu 24.04 LTS VM
build-essential
linux-headers-$(uname -r)
cgroup v2
/proc/pressure/cpu
/proc/pressure/memory
/proc/pressure/io
sudo/root 权限
```

WSL2 不作为内核模块和完整 cgroup 验证的参考环境。

## 快速开始

克隆仓库：

```bash
git clone https://github.com/int2t05/OSLab-TraceBench.git
cd OSLab-TraceBench
```

运行基础四模块验证：

```bash
bash tests/run_all.sh
```

在 Ubuntu VM 中验证 Linux 观测模块：

```bash
cd extension/oslab_monitor
bash tests/test_oslab_monitor.sh
```

在 Ubuntu VM 中验证 TraceBench：

```bash
cd extension/tracebench
sudo bash tests/test_tracebench.sh
```

## 基础模块

处理机调度：

```bash
cd basic/scheduler
make
./scheduler --algorithm fcfs < tests/sample.txt
./scheduler --algorithm sjf < tests/sample.txt
./scheduler --algorithm rr < tests/sample.txt
./scheduler --algorithm priority < tests/sample.txt
make clean
```

内存管理：

```bash
cd basic/memory
make
./memory --mode partition --algorithm ff < tests/partition.txt
./memory --mode partition --algorithm bf < tests/partition.txt
./memory --mode paging --algorithm fifo < tests/pages.txt
./memory --mode paging --algorithm lru < tests/pages.txt
make clean
```

进程同步：

```bash
cd basic/sync
make
./sync --problem producer_consumer --producers 2 --consumers 2 --buffer-size 4 --count 10
./sync --problem readers_writers --readers 3 --writers 2 --count 5
./sync --problem dining_philosophers --count 3
make clean
```

内存型文件系统：

```bash
cd basic/filesystem
make
./filesystem < tests/fs_commands.txt
make clean
```

## Linux 观测模块

构建并加载：

```bash
cd extension/oslab_monitor/kernel
make
sudo insmod oslab_monitor.ko
```

读取 `/proc` 接口：

```bash
cat /proc/oslab_monitor/overview
cat /proc/oslab_monitor/tasks
echo 1 | sudo tee /proc/oslab_monitor/pid
cat /proc/oslab_monitor/pid
```

使用用户态 CLI：

```bash
cd extension/oslab_monitor/user
make
./oslabctl --help
./oslabctl overview
./oslabctl tasks
sudo ./oslabctl pid 1
```

卸载模块：

```bash
cd extension/oslab_monitor/kernel
sudo rmmod oslab_monitor
make clean
```

## TraceBench

构建并查看帮助：

```bash
cd extension/tracebench
make
./tracebench --help
```

运行三类资源压力：

```bash
sudo ./tracebench run --profile cpu --duration 3 --sample-interval 1 --cpu-workers 2 --output output/cpu_run
sudo ./tracebench run --profile memory --duration 3 --sample-interval 1 --memory-mb 64 --output output/memory_run
sudo ./tracebench run --profile io --duration 3 --sample-interval 1 --io-mb 16 --output output/io_run
```

生成 Markdown 报告并清理运行态资源：

```bash
./tracebench report --input output/cpu_run --output output/cpu_run/report.md
sudo ./tracebench cleanup
```

每次 `tracebench run` 会写入：

```text
command.txt
environment.txt
samples.csv
summary.txt
```

## 仓库结构

```text
basic/                         OS 机制模型
basic/scheduler/               FCFS、SJF、RR 和优先级调度
basic/memory/                  动态分区和页面置换
basic/sync/                    pthread 同步工作负载
basic/filesystem/              内存型目录和块分配模型
extension/oslab_monitor/        Linux 内核模块和 /proc 用户态 CLI
extension/tracebench/           资源压力采样工具
tests/run_all.sh                基础用户态验证入口
docs/PRD.md                     核心系统需求
docs/PRDv2.md                   TraceBench 需求
docs/TECH.md                    核心系统技术设计
docs/TECHv2.md                  TraceBench 技术设计
docs/FEATURES.md                能力清单
docs/OPEN_SOURCE.md             开源上线信息和仓库主题建议
```

## 参考验证环境

```text
Ubuntu 24.04.2 LTS
Linux 6.11.0-17-generic
gcc 13.3.0
GNU Make 4.3
```

已验证命令：

```bash
bash tests/run_all.sh
cd extension/oslab_monitor
bash tests/test_oslab_monitor.sh
cd extension/tracebench
sudo bash tests/test_tracebench.sh
```

## 文档

- [需求文档](docs/PRD.md)
- [核心技术设计](docs/TECH.md)
- [TraceBench 需求](docs/PRDv2.md)
- [TraceBench 技术设计](docs/TECHv2.md)
- [能力清单](docs/FEATURES.md)
- [开源上线信息](docs/OPEN_SOURCE.md)

## 贡献

欢迎提交 Issue 和 Pull Request。请先阅读 [CONTRIBUTING.md](CONTRIBUTING.md)，并确保相关验证脚本可以通过。涉及内核模块、cgroup 或 `/proc` 行为的变更，应在 Ubuntu VM 中验证。

## 许可证

本项目采用 [木兰宽松许可证第 2 版](LICENSE)（SPDX: `MulanPSL-2.0`）。
