# PRD v2：TraceBench 资源压力观测

## 1. 目标

TraceBench 为 OSLab TraceBench 增加一个用户态工具，用于生成可控 Linux 资源压力并采样运行态信号。该工具运行 CPU、内存和 I/O 工作负载，记录 PSI 与 cgroup v2 计数器，可选采样 `/proc/oslab_monitor/overview`，并输出可复现的文本、CSV、摘要和 Markdown 材料。

TraceBench 实现在 `extension/tracebench/` 下，不修改基础 OS 模型，也不修改 `oslab_monitor` 内核模块。

## 2. 目标用户

- 需要对比不同工作负载下 OS 资源压力信号的研究者。
- 需要在 VM 中验证 Linux PSI 和 cgroup v2 观测行为的开发者。
- 需要从命令输入和生成材料复现 benchmark 运行的审阅者。

## 3. 范围

已实现基线范围：

- `tracebench --help`
- `tracebench run`
- `tracebench report`
- `tracebench cleanup`
- CPU、内存和 I/O 压力 profile。
- cgroup v2 分组和指标采集。
- 从 `/proc/pressure/*` 采集 PSI。
- 可选 `oslab_monitor` 对照采样。
- 稳定 `samples.csv` 输出。
- `command.txt`、`environment.txt` 和 `summary.txt` 元数据。
- Markdown 报告生成。
- Bash 集成验证。

非范围：

- Linux 内核源码修改。
- 宿主机调度器替换。
- GUI 或 Web UI。
- 强制依赖 `stress-ng` 或 `fio` 等外部压力工具。
- 强制依赖 `perf` 或 `bpftrace` 等外部追踪工具。
- 将 sched_ext 作为默认工作流。

## 4. 运行环境

参考环境：

```text
Ubuntu 22.04 LTS 或 Ubuntu 24.04 LTS VM
支持 cgroup v2 的 Linux 内核
/proc/pressure/cpu
/proc/pressure/memory
/proc/pressure/io
gcc
make
bash
默认 cgroup 模式需要 sudo/root
```

完整验证需要 cgroup v2 和 PSI 支持。`--no-cgroup` 可用于低权限探索，但不会产生完整的 cgroup 指标。

## 5. 命令行需求

可执行文件：

```text
extension/tracebench/tracebench
```

必需命令：

```bash
./tracebench --help
sudo ./tracebench run --profile cpu --duration 5 --sample-interval 1 --output output/cpu
sudo ./tracebench run --profile memory --duration 5 --sample-interval 1 --output output/memory
sudo ./tracebench run --profile io --duration 5 --sample-interval 1 --output output/io
./tracebench report --input output/cpu --output output/cpu/report.md
sudo ./tracebench cleanup
```

运行参数：

| 参数 | 要求 |
|---|---|
| `--profile` | 必填；取值为 `cpu`、`memory`、`io` |
| `--duration` | 必填正整数秒 |
| `--sample-interval` | 必填正整数秒，不能大于 duration |
| `--output` | `run` 命令的输出目录 |
| `--cpu-workers` | CPU profile 的工作线程数，默认 `2` |
| `--memory-mb` | 内存 profile 的内存大小，默认 `128` |
| `--io-mb` | I/O profile 的写入大小，默认 `64` |
| `--cgroup-name` | cgroup 命名空间，默认 `oslab_tracebench` |
| `--no-cgroup` | 禁用 cgroup 创建，cgroup 字段写为 `NA` |
| `--with-oslab-monitor` | 要求 `/proc/oslab_monitor/overview` 存在 |

报告参数：

| 参数 | 要求 |
|---|---|
| `--input` | 包含 `samples.csv` 的目录 |
| `--output` | Markdown 报告路径 |

所有错误都必须写入 `error:` 前缀，并返回非零退出码。

## 6. 工作负载需求

CPU profile：

- 启动 CPU-bound 工作线程。
- 支持配置工作线程数。
- 持续运行到配置的 duration 结束。

Memory profile：

- 分配配置的内存大小。
- 周期性触碰内存页，使内存压力可见。
- 默认不设置 cgroup 内存限制。

I/O profile：

- 在输出目录写入固定临时文件。
- 调用 `fsync` 暴露存储压力。
- 正常完成时删除临时文件。

所有工作负载必须由项目代码实现，并归属于 `tracebench` 进程树。

## 7. 采样需求

PSI：

- 读取 `/proc/pressure/cpu`。
- 读取 `/proc/pressure/memory`。
- 读取 `/proc/pressure/io`。
- 在可用时解析 `some` 和 `full` 行。
- 记录 `avg10`、`avg60`、`avg300` 和 `total`。

cgroup v2：

- 创建 `/sys/fs/cgroup/<cgroup-name>/<profile>/<run-id>/`。
- 将工作负载子进程移入运行 cgroup。
- 读取 `cpu.stat`。
- 读取 `memory.current`。
- 读取 `memory.events`。
- 删除本次运行创建的空 cgroup。

`oslab_monitor` 对照：

- 如果 `/proc/oslab_monitor/overview` 存在，采集 `total_tasks`、`running_tasks`、`sleeping_tasks`、`mem_free_kb` 和 `mem_available_kb`。
- 默认模式下如果不存在，记录 `oslab_monitor_available=false`。
- `--with-oslab-monitor` 模式下如果不存在，必须以 `error:` 失败。

## 8. 输出需求

每次运行在请求的输出目录中写入：

```text
command.txt
environment.txt
samples.csv
summary.txt
```

`tracebench report` 在请求路径写入 Markdown 报告。

`samples.csv` 必须有稳定表头。必需字段组：

- 样本序号和已运行时间。
- profile、duration、sample interval 和 run id。
- cgroup 启用标记和路径。
- CPU、内存和 I/O PSI 字段。
- cgroup CPU 和内存字段。
- `oslab_monitor` 可用性和选定计数器。

`summary.txt` 必须包含：

- profile。
- duration 和 sample interval。
- 输出路径。
- cgroup 状态。
- 样本数量。
- 生成文件引用。

Markdown 报告必须包含：

- 运行配置。
- 环境摘要。
- `samples.csv` 引用。
- PSI 摘要。
- cgroup 摘要。
- `oslab_monitor` 对照部分。
- 局限性和解释说明。

## 9. 清理需求

`tracebench cleanup` 只允许删除：

- `/sys/fs/cgroup/<cgroup-name>/` 下的 cgroup。
- TraceBench 输出 profile 目录下的 `tracebench_io.tmp` 文件。

不得删除 CSV、摘要、Markdown 报告、用户创建文件或任意目录。如果 cgroup 中仍有进程，cleanup 必须失败或跳过该 cgroup，而不能杀死无关进程。

## 10. 验证需求

集成脚本：

```bash
extension/tracebench/tests/test_tracebench.sh
```

必须验证：

- 构建成功。
- help 输出。
- 参数错误路径。
- CPU profile 运行。
- memory profile 运行。
- I/O profile 运行。
- CSV 表头稳定性。
- CSV 每行字段数量一致。
- summary 生成。
- Markdown 报告生成。
- cleanup 行为。
- proc 接口缺失时 `--with-oslab-monitor` 失败。

## 11. 完成清单

- [ ] `make` 可以构建 `tracebench`。
- [ ] `tracebench --help` 显示 `run`、`report` 和 `cleanup`。
- [ ] CPU profile 可以运行并退出。
- [ ] Memory profile 可以运行并退出。
- [ ] I/O profile 可以运行并退出。
- [ ] 每个 profile 写入 `samples.csv`。
- [ ] CSV 包含 PSI 字段。
- [ ] CSV 包含 cgroup 字段。
- [ ] CSV 包含 `oslab_monitor` 字段。
- [ ] `tracebench report` 生成 Markdown。
- [ ] `tracebench cleanup` 保留生成的 CSV、摘要和报告。
- [ ] 非法参数返回非零并输出 `error:`。
- [ ] 参考 VM 上 `sudo bash tests/test_tracebench.sh` 通过。
