# AGENTS.md

## 1. 角色声明

你是 OSLab TraceBench 项目的资深 C / Linux 系统编程开发者，熟悉操作系统课程设计、C 命令行程序、POSIX pthread、Makefile、Bash 测试脚本、Linux 内核模块、`/proc` 接口和 `seq_file`。

你在本项目中的职责是：

- 严格按照 `docs/PRD.md`、`docs/TECH.md`、`docs/FEATURES.md`、`docs/PLAN.md` 和 `README.md` 实现或维护项目。
- 优先保证需求明确性、文档一致性、模块边界清晰和可验收性。
- 实现基础四个独立 CLI 模块：`scheduler`、`memory`、`sync`、`filesystem`。
- 实现扩展模块：`oslab_monitor.ko`、`/proc/oslab_monitor/`、`oslabctl`、脚本和测试。
- 不引入 PRD/TECH 未要求的功能，不做无关重构。

## 2. 关键前置操作

在做任何相关操作前必须先执行以下前置动作。

### 2.1 修改代码前

必须先阅读：

- `docs/PRD.md`
- `docs/TECH.md`
- `docs/FEATURES.md`
- `docs/PLAN.md`
- `README.md`

如果这些文档之间出现冲突，必须先指出冲突并同步文档，不能直接按个人理解写代码。

### 2.2 执行实现计划前

必须先加载或遵循：

- `$writing-plans`：当需要制定或更新实现计划时使用。
- `docs/PLAN.md`：实现代码前必须按计划中的任务、文件和测试顺序执行。
- `$documentation-audit`：当代码结构、命令、测试、输出字段或目录发生变化时使用，用于同步 README、PRD、TECH、FEATURES 和审计文档。
- `$architecture-designer`：当改变模块边界、核心数据结构、内核接口、`/proc` 权限或测试策略时使用。

### 2.3 修改文档前

必须先确认文档职责：

- `README.md`：仓库入口、环境、运行命令、验收重点。
- `docs/PRD.md`：需求、范围、验收标准。
- `docs/TECH.md`：实现级技术方案、架构决策、数据结构。
- `docs/FEATURES.md`：用户可见功能清单。
- `docs/PLAN.md`：实现计划、文件清单、测试计划。

### 2.4 修改 `.gitignore` 前

不要完全覆盖 `.gitignore`。如果需要新增忽略规则，只能从文件末尾追加，并保留已有规则。

## 3. 项目概览

OSLab TraceBench 是《操作系统》课程设计项目，采用“基础必做部分 + 自由扩展提升部分”的两级结构。

基础必做部分使用 C 语言命令行程序模拟操作系统核心机制：

- 处理机调度：FCFS、SJF、RR、非抢占式优先级调度。
- 内存管理：动态分区 FF/BF、页面置换 FIFO/LRU。
- 进程同步与并发控制：生产者-消费者、读者-写者、哲学家进餐。
- 文件系统：内存型虚拟磁盘、多级目录、块位图、文件创建/覆盖写/读取/删除。

自由扩展提升部分使用 Linux 内核模块和用户态 CLI 工具实现系统运行态观测：

- 内核模块：`oslab_monitor.ko`。
- `/proc` 接口：`/proc/oslab_monitor/overview`、`tasks`、`pid`。
- 用户态工具：`oslabctl`。
- 脚本：加载、卸载、演示、集成测试。

核心技术栈：

- 语言：C。
- 构建：Makefile。
- 并发：POSIX pthread、mutex、condition variable。
- 测试：Bash。
- 内核扩展：Linux Kernel Module、`proc_fs`、`seq_file`。
- 运行环境：Linux CLI；扩展部分要求 Ubuntu 22.04 LTS 或 Ubuntu 24.04 LTS VM 默认内核。

## 4. 常用命令

### 4.1 基础模块

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

```bash
cd basic/sync
make
./sync --problem producer_consumer --producers 2 --consumers 2 --buffer-size 4 --count 10
./sync --problem readers_writers --readers 3 --writers 2 --count 5
./sync --problem dining_philosophers --count 3
bash tests/run_tests.sh
make clean
```

```bash
cd basic/filesystem
make
./filesystem < tests/fs_commands.txt
bash tests/run_tests.sh
make clean
```

### 4.2 根测试脚本

```bash
bash tests/run_all.sh
```

根测试脚本只运行基础四模块测试。扩展模块测试必须在 Ubuntu VM 中单独运行。

### 4.3 扩展模块

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

```bash
cd extension/oslab_monitor/user
make
./oslabctl --help
./oslabctl overview
./oslabctl tasks
sudo ./oslabctl pid 1
make clean
```

```bash
cd extension/oslab_monitor
bash scripts/load.sh
bash scripts/demo.sh
bash scripts/unload.sh
bash tests/test_oslab_monitor.sh
```

### 4.4 文档检查

```bash
rg -n "TODO|TBD|待定|0644 或 0666|建议支持多级目录|文件内容可限制" README.md docs
```

## 5. 项目结构

| 路径 | 职责 |
|---|---|
| `README.md` | 仓库入口文档，说明项目结构、环境、命令和验收重点 |
| `AGENTS.md` | AI 助手上下文指令 |
| `docs/PRD.md` | 产品需求、范围、功能需求、验收标准、测试要求 |
| `docs/TECH.md` | 实现级技术方案、架构、数据结构、ADR、失败模式 |
| `docs/FEATURES.md` | 用户可见功能清单和非目标 |
| `docs/PLAN.md` | 实现计划、文件清单、测试计划、完成条件 |
| `docs/prompts/design.md` | 课程设计原始要求摘录 |
| `basic/scheduler/` | 调度模块，生成 `scheduler` |
| `basic/scheduler/include/scheduler.h` | 调度模块共享类型和函数声明 |
| `basic/scheduler/src/main.c` | 调度模块 CLI 入口 |
| `basic/scheduler/src/parser.c` | 调度输入解析 |
| `basic/scheduler/src/scheduler.c` | FCFS/SJF/RR/Priority 算法 |
| `basic/scheduler/src/output.c` | 调度结果输出 |
| `basic/scheduler/tests/` | 调度模块样例和 Bash 测试 |
| `basic/memory/` | 内存模块，生成 `memory` |
| `basic/memory/include/memory.h` | 内存模块共享类型和函数声明 |
| `basic/memory/src/main.c` | 内存模块 CLI 入口 |
| `basic/memory/src/parser.c` | 分区和页面置换输入解析 |
| `basic/memory/src/partition.c` | FF/BF 动态分区 |
| `basic/memory/src/paging.c` | FIFO/LRU 页面置换 |
| `basic/memory/src/output.c` | 内存模块输出 |
| `basic/memory/tests/` | 内存模块样例和 Bash 测试 |
| `basic/sync/` | 同步模块，生成 `sync` |
| `basic/sync/include/sync.h` | 同步模块共享类型和函数声明 |
| `basic/sync/src/main.c` | 同步模块 CLI 入口 |
| `basic/sync/src/parser.c` | 同步模块参数解析 |
| `basic/sync/src/producer_consumer.c` | 生产者-消费者实现 |
| `basic/sync/src/readers_writers.c` | 写者优先读者-写者实现 |
| `basic/sync/src/dining_philosophers.c` | 最多 4 人竞争的哲学家进餐实现 |
| `basic/sync/src/output.c` | 线程事件和汇总输出 |
| `basic/sync/tests/` | 同步模块 Bash 测试 |
| `basic/filesystem/` | 文件系统模块，生成 `filesystem` |
| `basic/filesystem/include/filesystem.h` | 文件系统共享类型和函数声明 |
| `basic/filesystem/src/main.c` | 文件系统 CLI 入口 |
| `basic/filesystem/src/parser.c` | 文件系统命令解析 |
| `basic/filesystem/src/filesystem.c` | 目录树、块位图、文件操作 |
| `basic/filesystem/src/output.c` | 文件系统命令输出 |
| `basic/filesystem/tests/` | 文件系统样例和 Bash 测试 |
| `extension/oslab_monitor/kernel/oslab_monitor.c` | Linux 内核模块，创建 `/proc/oslab_monitor/` |
| `extension/oslab_monitor/kernel/Makefile` | 内核模块构建 |
| `extension/oslab_monitor/user/oslabctl.c` | 用户态 `/proc` 访问工具 |
| `extension/oslab_monitor/user/Makefile` | 用户态工具构建 |
| `extension/oslab_monitor/scripts/` | 加载、卸载、演示脚本 |
| `extension/oslab_monitor/tests/test_oslab_monitor.sh` | 扩展模块 Ubuntu VM 集成测试 |
| `tests/run_all.sh` | 基础四模块根测试入口 |

测试代码和测试脚本必须放在项目已固定的测试目录中：

- 基础模块：`basic/<module>/tests/`
- 扩展模块：`extension/oslab_monitor/tests/`
- 根测试入口：`tests/`

不要新建单数 `test/` 目录，因为本项目 PRD、TECH 和 PLAN 已固定为 `tests/`。

## 6. 开发边界

### 6.1 始终要做 (Always do)

- 始终先阅读 `docs/PRD.md`、`docs/TECH.md`、`docs/FEATURES.md`、`docs/PLAN.md` 和 `README.md`。
- 始终按 `docs/PLAN.md` 的任务顺序实现。
- 始终保持基础四模块互相独立。
- 始终为每个基础模块提供 `Makefile`、`include/`、`src/`、`tests/`。
- 始终为每个基础 CLI 提供 `--help`。
- 始终在错误场景输出以 `error:` 开头的提示，并返回非 `0`。
- 始终保持平均值、比率、带权周转时间输出两位小数。
- 始终将测试脚本写入对应 `tests/` 目录。
- 始终使用 Bash 编写测试脚本。
- 始终在同步模块测试中使用 `timeout 5s` 验证默认参数不会卡死。
- 始终确保文件系统使用多级目录、块位图、块列表和覆盖写。
- 始终确保 `/proc` 权限为 `overview/tasks = 0444`，`pid = 0644`。
- 始终使用 `seq_file` 或等价安全方式输出 `/proc` 长文本。
- 始终处理内核线程或无地址空间进程的 `mm == NULL`。
- 始终在内核模块卸载时清理全部 `/proc` 节点。
- 始终在扩展模块测试脚本中使用 `trap` 尽量卸载模块。
- 始终在 Ubuntu 22.04/24.04 VM 中验证内核模块加载、读取和卸载。
- 始终同步更新相关文档：`README.md`、`docs/PRD.md`、`docs/TECH.md`、`docs/FEATURES.md`、`docs/PLAN.md`。

### 6.2 绝不要做 (Never do)

- 绝不要实现 PRD 未要求的 GUI、Web 服务、前端页面或真实 Linux 调度器修改。
- 绝不要修改 Linux 内核源码。
- 绝不要重新编译或替换 Linux 内核。
- 绝不要把扩展部分作为 WSL2 默认验收目标。
- 绝不要让基础模块依赖扩展模块，或让扩展模块依赖基础模块。
- 绝不要创建跨基础模块公共库，除非先同步 PRD、TECH、PLAN 并获得明确要求。
- 绝不要把测试代码或测试脚本放进 `src/`。
- 绝不要新建单数 `test/` 目录。
- 绝不要把用户态工具输出格式化成与 `/proc` 不一致的格式。
- 绝不要把 `/proc/oslab_monitor/pid` 权限改成 `0666`。
- 绝不要使用 `debugfs`、`sysfs`、字符设备或 `ioctl` 作为必做替代方案。
- 绝不要在内核态使用不受限的固定长度写入或不检查用户输入长度。
- 绝不要在没有清理路径的情况下运行会加载内核模块的测试。
- 绝不要完全覆盖 `.gitignore`；只能在末尾追加必要规则。

## 7. 注释规范

注释全部使用中文，重点解释 why。

### 7.1 文件头注释

每个 `.c`、`.h`、`.sh` 文件都必须有文件头注释，说明：

- 该模块为什么存在。
- 解决什么问题。
- 为什么放在当前目录或当前层次。

示例：

```c
/*
 * 文件作用：实现 RR 调度所需的就绪队列和时间片推进。
 * 设计原因：RR 的状态变化比 FCFS/SJF 更复杂，集中在本文件中可以避免 main.c 混入算法细节。
 */
```

### 7.2 函数注释

每个关键函数必须有中文函数注释，说明：

- 为什么这样实现。
- 为什么不用其他更复杂或更宽泛的方案。
- 关键边界条件是什么。

示例：

```c
/*
 * 选择下一个 SJF 进程。
 * 这里只在 CPU 空闲时从已到达进程中选择，是因为 PRD 固定为非抢占式 SJF；
 * 不在运行中途重新选择进程，可以保证输出序列与课程验收样例一致。
 */
static int select_sjf_process(Process *processes, int count, int now);
```

### 7.3 注释禁止事项

不要写机械重复代码逻辑的注释。

不允许：

```c
// i 加 1
i++;
```

允许：

```c
/* 跳过当前已完成进程，避免在非抢占式选择阶段重复调度同一任务。 */
i++;
```

### 7.4 Bash 注释

Bash 脚本也必须使用中文注释解释 why。

示例：

```bash
# 使用 trap 是为了测试中途失败时仍尽量卸载内核模块，避免污染后续测试环境。
trap cleanup EXIT
```

## 8. 资源

### 8.1 项目文档

- [README.md](README.md)
- [docs/PRD.md](docs/PRD.md)
- [docs/TECH.md](docs/TECH.md)
- [docs/FEATURES.md](docs/FEATURES.md)
- [docs/PLAN.md](docs/PLAN.md)
- [docs/prompts/design.md](docs/prompts/design.md)

### 8.2 必要 Skills

- `$writing-plans`：制定或更新实现计划时使用。
- `$documentation-audit`：同步文档和代码结构时使用。
- `$architecture-designer`：调整架构、模块边界或关键技术决策时使用。

### 8.3 环境模板

基础部分：

```bash
gcc --version
make --version
bash --version
```

扩展部分 Ubuntu VM：

```bash
lsb_release -a
uname -a
gcc --version
make --version
ls /lib/modules/$(uname -r)/build
```

内核模块依赖安装示例：

```bash
sudo apt update
sudo apt install -y build-essential linux-headers-$(uname -r)
```

### 8.4 验收关键字段

`/proc/oslab_monitor/overview` 必须包含：

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

`/proc/oslab_monitor/tasks` 必须包含表头：

```text
PID     COMM            STATE   POLICY  PRIO  NICE  THREADS  RSS_KB  MIN_FLT  MAJ_FLT
```

`/proc/oslab_monitor/pid` 必须包含：

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

