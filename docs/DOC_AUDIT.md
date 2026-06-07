# 文档一致性审计

审计时间：2026-06-07

## 1. 审计范围

本次审计覆盖：

- `README.md`
- `docs/PRD.md`
- `docs/TECH.md`
- `docs/FEATURES.md`
- `docs/PLAN.md`
- `docs/REPORT_OUTLINE.md`
- `docs/DOC_AUDIT.md`
- `basic/`
- `extension/oslab_monitor/`
- `tests/run_all.sh`

## 2. 当前实现状态

| 范围 | 当前状态 | 证据 |
|---|---|---|
| 调度模块 | 已实现并通过测试 | `basic/scheduler/`，`bash basic/scheduler/tests/run_tests.sh` |
| 内存模块 | 已实现并通过测试 | `basic/memory/`，`bash basic/memory/tests/run_tests.sh` |
| 文件系统模块 | 已实现并通过测试 | `basic/filesystem/`，`bash basic/filesystem/tests/run_tests.sh` |
| 同步模块 | 已实现并通过测试 | `basic/sync/`，`bash basic/sync/tests/run_tests.sh` |
| 根测试入口 | 已实现并通过测试 | `tests/run_all.sh`，`bash tests/run_all.sh` |
| 扩展内核模块源码 | 已实现，待 Ubuntu VM 编译加载验证 | `extension/oslab_monitor/kernel/oslab_monitor.c` |
| 用户态工具 | 已实现，当前环境已验证编译和 `--help` | `extension/oslab_monitor/user/oslabctl.c` |
| 扩展脚本和集成测试 | 已实现，待 Ubuntu VM 运行 | `extension/oslab_monitor/scripts/`，`extension/oslab_monitor/tests/test_oslab_monitor.sh` |

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

根测试：

- `tests/run_all.sh`

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
| 扩展 Ubuntu VM 验证 | 当前环境不是 Ubuntu VM 默认内核验收环境 | 待验证 |

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

已执行但因环境限制未通过：

```bash
cd extension/oslab_monitor
bash tests/test_oslab_monitor.sh
```

失败原因：

```text
/lib/modules/5.15.123.1-microsoft-standard-WSL2/build: No such file or directory
```

该失败与 PRD/TECH 一致：扩展部分不以 WSL2 作为默认验收环境，必须在 Ubuntu 22.04/24.04 VM 中验证。

## 6. 待补充报告材料

- 基础四模块运行截图或完整输出文本。
- `bash tests/run_all.sh` 输出截图或文本。
- Ubuntu VM 中 `oslab_monitor.ko` 编译、加载、读取 `/proc`、卸载截图或文本。
- Ubuntu VM 中 `oslabctl overview`、`oslabctl tasks`、`sudo ./oslabctl pid 1` 输出截图或文本。

## 7. 审计结论

基础部分代码、测试和文档当前一致。扩展部分源码、用户态工具、脚本和测试脚本已按文档实现；最终运行验收仍需在 Ubuntu 22.04/24.04 VM 默认内核中完成。
