# PRD: OSLab TraceBench Core System

## 1. Objective

OSLab TraceBench provides deterministic user-space models for operating-system mechanisms and a Linux kernel observability extension. The core system is designed for reproducible command-line execution, stable text output, and integration with automated validation scripts.

The core system consists of:

- processor scheduling model.
- memory-management model.
- synchronization workloads.
- in-memory filesystem model.
- Linux `/proc` observability module.
- `oslabctl` user-space wrapper.

## 2. Users

- Researchers studying operating-system mechanisms with deterministic inputs.
- Developers validating process and memory observability through Linux kernel modules.
- Reviewers reproducing model and observability outputs from a clean checkout.

## 3. Scope

Included:

- Four independent user-space C modules under `basic/`.
- One Linux kernel module under `extension/oslab_monitor/kernel/`.
- One user-space CLI under `extension/oslab_monitor/user/`.
- Bash validation scripts and deterministic input fixtures.

Excluded:

- Linux kernel source modification.
- Host scheduler replacement.
- Production filesystem implementation.
- GUI or web frontend.
- Windows-native kernel-module workflow.
- WSL2 as the reference kernel-module validation environment.

## 4. Runtime Environment

User-space modules:

```text
Linux
gcc
make
bash
pthread
```

Kernel module:

```text
Ubuntu 22.04 LTS or Ubuntu 24.04 LTS VM
build-essential
linux-headers-$(uname -r)
insmod
rmmod
sudo/root for module loading
```

## 5. Repository Structure

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

Each module owns its source, headers, build file, deterministic inputs, and validation script.

## 6. Global Requirements

- All user-space programs are implemented in C.
- Each module builds with `make` and cleans with `make clean`.
- Normal completion returns exit code `0`.
- Invalid parameters or malformed input return non-zero.
- Error messages start with `error:`.
- Output fields remain stable for automated parsing.
- Floating-point averages and rates use two decimal places where applicable.
- Modules do not depend on one another unless explicitly stated.

## 7. Processor Scheduling

Path: `basic/scheduler/`

Executable: `scheduler`

Command:

```bash
./scheduler --algorithm fcfs|sjf|rr|priority < tests/sample.txt
```

Requirements:

- Parse process count, time quantum, process name, arrival time, burst time, and priority.
- Implement FCFS.
- Implement non-preemptive SJF.
- Implement round-robin.
- Implement non-preemptive priority scheduling.
- Emit a stable timeline.
- Emit per-process start, finish, waiting, turnaround, and weighted-turnaround values.
- Emit average waiting, turnaround, and weighted-turnaround values.
- Emit `IDLE` segments when no process is runnable.

Input rules:

- `process_count > 0`.
- `time_quantum > 0` for round-robin.
- `arrival >= 0`.
- `burst > 0`.
- Lower priority value means higher scheduling priority.

## 8. Memory Management

Path: `basic/memory/`

Executable: `memory`

Commands:

```bash
./memory --mode partition --algorithm ff|bf < tests/partition.txt
./memory --mode paging --algorithm fifo|lru < tests/pages.txt
```

Partition requirements:

- Parse total memory size.
- Allocate named blocks.
- Free named blocks.
- Implement first-fit.
- Implement best-fit.
- Split allocated regions.
- Coalesce adjacent free regions.
- Emit allocated and free partition tables in address order.

Paging requirements:

- Parse frame count and reference string.
- Implement FIFO replacement.
- Implement LRU replacement.
- Emit frame state for each access.
- Emit page-fault count and fault rate.

## 9. Synchronization

Path: `basic/sync/`

Executable: `sync`

Commands:

```bash
./sync --problem producer_consumer --producers 2 --consumers 2 --buffer-size 4 --count 10
./sync --problem readers_writers --readers 3 --writers 2 --count 5
./sync --problem dining_philosophers --count 3
```

Requirements:

- Use POSIX pthreads.
- Use mutexes and condition variables or equivalent synchronization primitives.
- Implement producer-consumer with bounded buffer.
- Implement readers-writers with writer preference.
- Implement dining philosophers with deadlock avoidance.
- Emit thread actions and final summary counters.
- Complete finite workloads without deadlock.

## 10. In-Memory Filesystem

Path: `basic/filesystem/`

Executable: `filesystem`

Command:

```bash
./filesystem < tests/fs_commands.txt
```

Required operations:

- `mkfs DISK_SIZE BLOCK_SIZE`
- `mkdir PATH`
- `create PATH`
- `write PATH CONTENT`
- `read PATH`
- `ls PATH`
- `delete PATH`
- `stat`

Requirements:

- Use an in-memory virtual disk.
- Use fixed-size blocks.
- Track free blocks with a bitmap.
- Support multi-level directories.
- Store file block lists.
- Overwrite file contents on `write`.
- Release all blocks on `delete`.
- Preserve old file contents if a write cannot be completed.

## 11. Linux Observability Module

Path: `extension/oslab_monitor/`

Kernel module:

```text
extension/oslab_monitor/kernel/oslab_monitor.c
```

User CLI:

```text
extension/oslab_monitor/user/oslabctl.c
```

Proc entries:

```text
/proc/oslab_monitor/overview
/proc/oslab_monitor/tasks
/proc/oslab_monitor/pid
```

Permissions:

| Entry | Permission |
|---|---:|
| `overview` | `0444` |
| `tasks` | `0444` |
| `pid` | `0644` |

`overview` requirements:

- module name.
- kernel release.
- total task count.
- running, sleeping, stopped, and zombie task counts.
- total, free, and available memory.
- read-time jiffies.

`tasks` requirements:

- stable header.
- PID.
- command name.
- process state.
- scheduling policy.
- priority and nice value.
- thread count.
- RSS.
- minor and major fault fields where available.

`pid` requirements:

- root-writable target PID.
- readable details for selected PID.
- not-found response for missing processes.
- bounded user input handling.

`oslabctl` requirements:

- `oslabctl --help`
- `oslabctl overview`
- `oslabctl tasks`
- `sudo oslabctl pid <PID>`
- raw forwarding of proc output.
- clear errors when the module is absent or permissions are insufficient.

## 12. Validation Requirements

User-space validation:

```bash
bash tests/run_all.sh
```

Kernel-module validation in Ubuntu VM:

```bash
cd extension/oslab_monitor
bash tests/test_oslab_monitor.sh
```

Validation scripts must:

- build target binaries.
- check required output fields.
- exercise successful and failing inputs.
- clean build products.
- avoid hard-coding machine-specific process counts or memory totals.

## 13. Acceptance Checklist

- [ ] `basic/scheduler` builds and validates all scheduling policies.
- [ ] `basic/memory` builds and validates partition and paging modes.
- [ ] `basic/sync` builds and validates all synchronization workloads.
- [ ] `basic/filesystem` builds and validates all required filesystem operations.
- [ ] `tests/run_all.sh` validates all user-space modules.
- [ ] `oslab_monitor.ko` builds in the reference VM.
- [ ] `/proc/oslab_monitor/overview` is readable.
- [ ] `/proc/oslab_monitor/tasks` is readable.
- [ ] `/proc/oslab_monitor/pid` accepts a root-written PID and exposes details.
- [ ] `oslabctl` wraps all proc interfaces.
- [ ] unloading the module removes `/proc/oslab_monitor/`.
