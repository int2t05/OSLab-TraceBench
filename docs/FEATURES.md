# Capability Index

This document summarizes the implemented capabilities of OSLab TraceBench. The repository provides deterministic operating-system mechanism models, a Linux `/proc` observability module, and a resource-pressure tracing tool for reproducible Linux runtime analysis.

## Validation Status

- `basic/` user-space modules are validated by `bash tests/run_all.sh`.
- `extension/oslab_monitor/` is validated on Ubuntu VM by `bash tests/test_oslab_monitor.sh`.
- `extension/tracebench/` is validated on Ubuntu VM by `sudo bash tests/test_tracebench.sh`.

## 1. User-Space OS Models

The `basic/` directory contains four independent C command-line programs. Each module has its own `Makefile`, source directory, headers, reproducible input files, and validation script.

### 1.1 Processor Scheduling

Path: `basic/scheduler/`

Executable: `scheduler`

Capabilities:

- FCFS scheduling.
- Non-preemptive SJF scheduling.
- Round-robin scheduling.
- Non-preemptive priority scheduling.
- Stable output for timeline, per-process metrics, average waiting time, average turnaround time, and average weighted turnaround time.

Commands:

```bash
./scheduler --algorithm fcfs < tests/sample.txt
./scheduler --algorithm sjf < tests/sample.txt
./scheduler --algorithm rr < tests/sample.txt
./scheduler --algorithm priority < tests/sample.txt
```

### 1.2 Memory Management

Path: `basic/memory/`

Executable: `memory`

Capabilities:

- Dynamic partition allocation with first-fit and best-fit.
- Allocation, release, and adjacent free-block coalescing.
- Page replacement with FIFO and LRU.
- Stable output for partition state, frame state, page faults, and fault rate.

Commands:

```bash
./memory --mode partition --algorithm ff < tests/partition.txt
./memory --mode partition --algorithm bf < tests/partition.txt
./memory --mode paging --algorithm fifo < tests/pages.txt
./memory --mode paging --algorithm lru < tests/pages.txt
```

### 1.3 Synchronization Workloads

Path: `basic/sync/`

Executable: `sync`

Capabilities:

- Producer-consumer synchronization.
- Readers-writers synchronization with writer preference.
- Dining-philosophers synchronization with deadlock avoidance.
- pthread-based concurrency with stable summary counters.

Commands:

```bash
./sync --problem producer_consumer --producers 2 --consumers 2 --buffer-size 4 --count 10
./sync --problem readers_writers --readers 3 --writers 2 --count 5
./sync --problem dining_philosophers --count 3
```

### 1.4 In-Memory Filesystem

Path: `basic/filesystem/`

Executable: `filesystem`

Capabilities:

- In-memory virtual disk.
- Multi-level directory tree.
- Block bitmap allocation.
- File create, overwrite, read, and delete operations.
- Directory listing and filesystem statistics.

Command:

```bash
./filesystem < tests/fs_commands.txt
```

Supported operations:

- `mkfs DISK_SIZE BLOCK_SIZE`
- `mkdir PATH`
- `create PATH`
- `write PATH CONTENT`
- `read PATH`
- `ls PATH`
- `delete PATH`
- `stat`

## 2. Linux Runtime Observability

The `extension/oslab_monitor/` module exposes Linux process and memory state through `/proc/oslab_monitor/` and provides a small user-space CLI wrapper.

### 2.1 Kernel Module

Path: `extension/oslab_monitor/kernel/`

Module: `oslab_monitor.ko`

Capabilities:

- Creates `/proc/oslab_monitor/` when loaded.
- Exposes `overview`, `tasks`, and `pid` proc entries.
- Cleans all proc entries when unloaded.
- Uses stable text fields for downstream parsing.

Commands:

```bash
make
sudo insmod oslab_monitor.ko
sudo rmmod oslab_monitor
```

### 2.2 `/proc/oslab_monitor/overview`

Capabilities:

- Module name and kernel version.
- Process-state counts.
- System memory totals.
- Read-time jiffies.

Command:

```bash
cat /proc/oslab_monitor/overview
```

### 2.3 `/proc/oslab_monitor/tasks`

Capabilities:

- Full process traversal.
- PID, command name, state, scheduling policy, priority, nice value, thread count, RSS, and fault fields.

Command:

```bash
cat /proc/oslab_monitor/tasks
```

### 2.4 `/proc/oslab_monitor/pid`

Capabilities:

- Root-writable target PID selection.
- Per-PID process details.
- Explicit not-found behavior.

Permissions:

- `overview/tasks = 0444`
- `pid = 0644`

Commands:

```bash
echo 1 | sudo tee /proc/oslab_monitor/pid
cat /proc/oslab_monitor/pid
```

### 2.5 `oslabctl`

Path: `extension/oslab_monitor/user/`

Executable: `oslabctl`

Capabilities:

- `oslabctl --help`
- `oslabctl overview`
- `oslabctl tasks`
- `sudo oslabctl pid <PID>`
- Clear errors when the module is not loaded, parameters are invalid, or permissions are insufficient.

## 3. TraceBench

TraceBench is implemented in `extension/tracebench/` as a user-space C tool for controlled resource pressure and runtime sampling.

### 3.1 Command Surface

Executable:

```text
extension/tracebench/tracebench
```

Commands:

```bash
./tracebench --help
sudo ./tracebench run --profile cpu --duration 3 --sample-interval 1 --cpu-workers 2 --output output/cpu_run
sudo ./tracebench run --profile memory --duration 3 --sample-interval 1 --memory-mb 64 --output output/memory_run
sudo ./tracebench run --profile io --duration 3 --sample-interval 1 --io-mb 16 --output output/io_run
./tracebench report --input output/cpu_run --output output/cpu_run/report.md
sudo ./tracebench cleanup
```

### 3.2 Runtime Capabilities

- CPU, memory, and I/O pressure profiles.
- cgroup v2 run grouping.
- PSI sampling from `/proc/pressure/cpu`, `/proc/pressure/memory`, and `/proc/pressure/io`.
- cgroup metric sampling from `cpu.stat`, `memory.current`, and `memory.events`.
- Optional comparison with `/proc/oslab_monitor/overview`.
- CSV output through `samples.csv`.
- Run metadata through `command.txt` and `environment.txt`.
- Summary output through `summary.txt`.
- Markdown report generation through `tracebench report`.
- Runtime cleanup through `tracebench cleanup`.

## 4. Non-Goals

The repository does not include:

- GUI or web frontend.
- Linux kernel source modification.
- Host scheduler replacement.
- Production filesystem implementation.
- Mandatory dependency on `stress-ng`, `fio`, `perf`, `bpftrace`, Prometheus, or Grafana.
- Windows-native validation for kernel-module or cgroup workflows.
