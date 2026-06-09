# 文档一致性审计

审计时间：2026-06-08

最近更新：

- 项目展示名已更新为 `OSLab TraceBench`。
- GitHub 仓库名计划同步为 `OSLab-TraceBench`。
- 新增 v2 需求文档 `docs/PRDv2.md`，规划 Linux 资源压力、cgroup v2、PSI、tracefs/bpftrace 和自动报告方向。
- 新增 v2 技术方案 `docs/TECHv2.md`。
- 新增 v2 实现计划 `docs/PLANv2.md`。
- 新增 v2 报告模板 `docs/TRACEBENCH_REPORT_TEMPLATE.md`。
- 新增并实现 `extension/tracebench/` TraceBench v2 P0 用户态实验工具。
- TraceBench v2 P0 已在 Ubuntu 24.04.2 LTS VM 中通过 `sudo bash tests/test_tracebench.sh`。

## 1. 审计范围

本次审计覆盖：

- `README.md`
- `docs/PRD.md`
- `docs/TECH.md`
- `docs/FEATURES.md`
- `docs/PLAN.md`
- `docs/REPORT_OUTLINE.md`
- `docs/COURSE_REPORT.md`
- `docs/PRDv2.md`
- `docs/TECHv2.md`
- `docs/PLANv2.md`
- `docs/TRACEBENCH_REPORT_TEMPLATE.md`
- `docs/DOC_AUDIT.md`
- `basic/`
- `extension/oslab_monitor/`
- `extension/tracebench/` 是否存在
- `tests/run_all.sh`
- `.gitattributes`

## 2. 当前实现状态

| 范围 | 当前状态 | 证据 |
|---|---|---|
| 调度模块 | 已实现并通过测试 | `basic/scheduler/`，`bash basic/scheduler/tests/run_tests.sh` |
| 内存模块 | 已实现并通过测试 | `basic/memory/`，`bash basic/memory/tests/run_tests.sh` |
| 文件系统模块 | 已实现并通过测试 | `basic/filesystem/`，`bash basic/filesystem/tests/run_tests.sh` |
| 同步模块 | 已实现并通过测试 | `basic/sync/`，`bash basic/sync/tests/run_tests.sh` |
| 根测试入口 | 已实现并通过测试 | `tests/run_all.sh`，`bash tests/run_all.sh` |
| 扩展内核模块源码 | 已实现并通过 Ubuntu VM 编译加载验证 | `extension/oslab_monitor/kernel/oslab_monitor.c`，`bash extension/oslab_monitor/tests/test_oslab_monitor.sh` |
| 用户态工具 | 已实现并通过 Ubuntu VM 集成测试 | `extension/oslab_monitor/user/oslabctl.c`，`oslabctl overview/tasks/pid` |
| 扩展脚本和集成测试 | 已实现并通过 Ubuntu VM 运行 | `extension/oslab_monitor/scripts/`，`extension/oslab_monitor/tests/test_oslab_monitor.sh` |
| 课程设计报告 | 已撰写详细报告 | `docs/COURSE_REPORT.md` |
| v2 需求文档 | 已撰写明确需求 | `docs/PRDv2.md` |
| v2 技术方案 | 已撰写实现级方案 | `docs/TECHv2.md` |
| v2 实现计划 | 已撰写任务、文件和测试计划 | `docs/PLANv2.md` |
| v2 报告模板 | 已创建报告结构模板 | `docs/TRACEBENCH_REPORT_TEMPLATE.md` |
| v2 TraceBench 实现 | 已实现并通过 Ubuntu VM 集成测试 | `extension/tracebench/`，`sudo bash extension/tracebench/tests/test_tracebench.sh` |
| 仓库展示 README | 已简化为运行说明入口 | `README.md` |

## 3. 新增源码和测试目录

基础部分：

- `basic/scheduler/Makefile`
- `basic/scheduler/include/scheduler.h`
- `basic/scheduler/src/main.c`
- `basic/scheduler/src/parser.c`
- `basic/scheduler/src/scheduler.c`
- `basic/scheduler/src/output.c`
- `basic/scheduler/tests/`
- `basic/memory/Makefile`
- `basic/memory/include/memory.h`
- `basic/memory/src/`
- `basic/memory/tests/`
- `basic/filesystem/Makefile`
- `basic/filesystem/include/filesystem.h`
- `basic/filesystem/src/`
- `basic/filesystem/tests/`
- `basic/sync/Makefile`
- `basic/sync/include/sync.h`
- `basic/sync/src/`
- `basic/sync/tests/run_tests.sh`

扩展部分：

- `extension/oslab_monitor/kernel/Makefile`
- `extension/oslab_monitor/kernel/oslab_monitor.c`
- `extension/oslab_monitor/user/Makefile`
- `extension/oslab_monitor/user/oslabctl.c`
- `extension/oslab_monitor/scripts/load.sh`
- `extension/oslab_monitor/scripts/unload.sh`
- `extension/oslab_monitor/scripts/demo.sh`
- `extension/oslab_monitor/tests/test_oslab_monitor.sh`
- `extension/tracebench/Makefile`
- `extension/tracebench/include/tracebench.h`
- `extension/tracebench/src/`
- `extension/tracebench/scripts/run_cpu_demo.sh`
- `extension/tracebench/scripts/run_memory_demo.sh`
- `extension/tracebench/scripts/run_io_demo.sh`
- `extension/tracebench/scripts/cleanup.sh`
- `extension/tracebench/tests/test_tracebench.sh`
- `extension/tracebench/bpftrace/README.md`
- `extension/tracebench/output/.gitkeep`

根测试：

- `tests/run_all.sh`
- `.gitattributes`

文档：

- `docs/COURSE_REPORT.md`
- `docs/PRDv2.md`
- `docs/TECHv2.md`
- `docs/PLANv2.md`
- `docs/TRACEBENCH_REPORT_TEMPLATE.md`

## 4. 文档与代码一致性

| 文档要求 | 当前代码状态 | 结论 |
|---|---|---|
| 基础四模块独立 CLI | 四个模块均有独立 Makefile、include、src、tests | 一致 |
| 所有基础 CLI 支持 `--help` | `scheduler`、`memory`、`sync`、`filesystem` 均已验证 | 一致 |
| 错误输出以 `error:` 开头 | 各模块测试覆盖非法输入 | 一致 |
| 调度固定 oracle | `basic/scheduler/tests/run_tests.sh` 覆盖 FCFS/SJF/RR/Priority | 一致 |
| 页面置换固定 oracle | `basic/memory/tests/run_tests.sh` 覆盖 FIFO/LRU 缺页次数和缺页率 | 一致 |
| 同步默认场景自动结束 | `basic/sync/tests/run_tests.sh` 使用 `timeout 5s` | 一致 |
| 文件系统多级目录、块位图、覆盖写 | `basic/filesystem/src/filesystem.c` 和测试样例覆盖 | 一致 |
| `/proc` 权限 | `overview/tasks = 0444`，`pid = 0644` | 一致 |
| `/proc` 长输出使用 `seq_file` | `overview`、`tasks`、`pid` 均使用 `seq_file` | 一致 |
| `oslabctl` 原样输出 `/proc` | 用户态工具直接读取并打印 proc 文件 | 一致 |
| 扩展 Ubuntu VM 验证 | Ubuntu 24.04.2 LTS VM 已通过内核模块集成测试 | 一致 |
| 脚本和 Makefile 行尾 | `.gitattributes` 固定 `*.sh` 和 `Makefile` 为 LF | 一致 |
| 课程报告 | `docs/COURSE_REPORT.md` 覆盖项目概述、环境、设计、测试、问题和总结 | 一致 |
| v2 需求文档 | `docs/PRDv2.md` 明确 TraceBench 的 P0/P1/P2 需求、命令、输出和验收标准 | 一致 |
| v2 技术方案 | `docs/TECHv2.md` 明确 TraceBench P0 模块边界、数据结构、CSV 字段、权限和测试策略 | 一致 |
| v2 实现计划 | `docs/PLANv2.md` 说明已执行的文件、脚本、测试和验证顺序 | 一致 |
| v2 报告模板 | `docs/TRACEBENCH_REPORT_TEMPLATE.md` 覆盖实验配置、环境、采样、PSI、cgroup、oslab_monitor 和局限性 | 一致 |
| v2 文档与代码状态 | v2 P0 文档、代码、测试脚本和报告模板均已落地 | 一致 |
| README 展示面 | README 保留项目简介、环境、运行命令和报告入口，去除长篇设计字段 | 一致 |

## 5. 已执行验证

当前环境：

```text
Linux DESKTOP-0M7QODM 5.15.123.1-microsoft-standard-WSL2
gcc (Ubuntu 11.4.0-1ubuntu1~22.04.2) 11.4.0
GNU Make 4.3
```

已执行并通过：

```bash
bash tests/run_all.sh
cd extension/oslab_monitor/user
make
./oslabctl --help
make clean
bash -n extension/oslab_monitor/scripts/load.sh extension/oslab_monitor/scripts/unload.sh extension/oslab_monitor/scripts/demo.sh extension/oslab_monitor/tests/test_oslab_monitor.sh
```

扩展脚本说明：

- `demo.sh` 和 `test_oslab_monitor.sh` 在 `set -euo pipefail` 下避免使用 `cat ... | head` 直接截断长输出，防止 `SIGPIPE` 将正常演示误判为失败。

Ubuntu VM 环境：

```text
Ubuntu 24.04.2 LTS
Linux ubuntu2404 6.11.0-17-generic
gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
GNU Make 4.3
/lib/modules/6.11.0-17-generic/build present
```

Ubuntu VM 已执行并通过：

```bash
bash tests/run_all.sh
cd extension/oslab_monitor
bash tests/test_oslab_monitor.sh
cd extension/tracebench
sudo bash tests/test_tracebench.sh
```

v2 文档审计已执行以下检查：

- 检查未决占位词和过时 `/proc` 权限表述。
- 检查 `TECHv2`、`PLANv2`、`TRACEBENCH_REPORT_TEMPLATE`、`extension/tracebench` 和 v2 实现状态引用。

说明：

- 本次审计为文档同步，不运行 v1 代码测试。
- `tests/run_all.sh` 继续只运行基础四模块，不纳入 TraceBench，因为 TraceBench 需要 Ubuntu VM、root 权限、cgroup v2 和 `/proc/pressure/*`。

扩展集成测试覆盖：

- `oslab_monitor.ko` 编译。
- `sudo insmod oslab_monitor.ko` 加载模块。
- `/proc/oslab_monitor/overview`、`tasks`、`pid` 字段检查。
- `oslabctl overview`、`oslabctl tasks`、`sudo oslabctl pid 1`。
- `sudo rmmod oslab_monitor` 卸载并确认 `/proc/oslab_monitor/` 清理。

曾在 WSL2 环境执行但因环境限制未通过：

```bash
cd extension/oslab_monitor
bash tests/test_oslab_monitor.sh
```

失败原因：

```text
/lib/modules/5.15.123.1-microsoft-standard-WSL2/build: No such file or directory
```

该失败与 PRD/TECH 一致：扩展部分不以 WSL2 作为默认验收环境；最终验收已在 Ubuntu 24.04.2 LTS VM 中完成。

## 6. 报告材料状态

- `docs/COURSE_REPORT.md` 已包含课程报告正文。
- 基础四模块测试结果已在报告中以文本形式记录。
- Ubuntu VM 中扩展模块编译、加载、读取 `/proc`、`oslabctl`、卸载测试结果已在报告中以文本形式记录。
- `docs/TRACEBENCH_REPORT_TEMPLATE.md` 已提供 v2 实验报告模板。
- 若课程提交要求必须使用截图，可按 README 中命令重新运行并截图。

## 7. 审计结论

基础部分代码、测试和文档当前一致。扩展部分 `oslab_monitor` 源码、用户态工具、脚本和测试脚本已按文档实现，并已在 Ubuntu 24.04.2 LTS VM 默认内核中通过完整集成测试。当前 README 已调整为简洁运行入口，详细课程设计报告已写入 `docs/COURSE_REPORT.md`。

v2 TraceBench P0 当前已实现：`extension/tracebench/` 包含 CLI、workload、PSI/cgroup/oslab_monitor 采样、CSV、summary、Markdown report、cleanup、demo scripts 和 P0 集成测试；`docs/PRDv2.md`、`docs/TECHv2.md`、`docs/PLANv2.md`、`docs/TRACEBENCH_REPORT_TEMPLATE.md`、`README.md` 和 `docs/FEATURES.md` 已同步为当前实现状态。
