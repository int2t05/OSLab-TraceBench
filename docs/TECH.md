# TECH: Core System Technical Design

## 1. Architecture

The core system is split into two layers:

- user-space OS mechanism models under `basic/`.
- Linux runtime observability under `extension/oslab_monitor/`.

The user-space models are independent C command-line programs. They share no common library and can be built, executed, and validated separately.

The observability extension is a Linux kernel module that exposes process and memory state through `/proc/oslab_monitor/`, plus a small C CLI that forwards proc output to userspace.

## 2. Build Model

Each module owns a `Makefile` with:

```bash
make
make clean
```

Common user-space compiler expectations:

```text
gcc
-Wall
-Wextra
C11-compatible code
```

Synchronization code links pthreads. Kernel module build uses the host kernel build system through the local `linux-headers-$(uname -r)` tree.

## 3. User-Space Module Layout

Each basic module follows:

```text
basic/<module>/
├── Makefile
├── include/
│   └── <module>.h
├── src/
│   ├── main.c
│   ├── parser.c
│   ├── <module>.c
│   └── output.c
└── tests/
```

Roles:

- `main.c`: CLI dispatch and top-level error handling.
- `parser.c`: text input parsing and validation.
- core implementation file: algorithm or mechanism logic.
- `output.c`: stable output formatting.
- `include/*.h`: shared types and function declarations inside the module.

## 4. Scheduler Design

Path: `basic/scheduler/`

Core structures:

- `Process`: name, arrival time, burst time, priority, remaining time, start/finish timestamps, input order.
- `Segment`: timeline segment name and time interval.
- `Timeline`: dynamic segment array.

Algorithms:

- FCFS sorts by arrival time and input order.
- SJF is non-preemptive and selects the shortest arrived unfinished process.
- Priority is non-preemptive and treats lower numeric priority as higher priority.
- RR uses a ready queue and time quantum.

The scheduler records `IDLE` timeline segments when the CPU has no runnable process.

Statistics:

```text
turnaround = finish - arrival
waiting = turnaround - burst
weighted_turnaround = turnaround / burst
```

## 5. Memory Design

Path: `basic/memory/`

Partition mode:

- linked-list partition representation.
- first-fit and best-fit search strategies.
- split-on-allocate.
- coalesce-on-free.
- allocated and free tables emitted by ascending start address.

Paging mode:

- array-backed frames.
- FIFO uses load timestamps.
- LRU uses last-use timestamps.
- empty frames are represented by `-1`.
- page-fault rate is calculated as `faults / accesses * 100`.

## 6. Synchronization Design

Path: `basic/sync/`

Producer-consumer:

- mutex.
- `not_full` condition variable.
- `not_empty` condition variable.
- ring buffer.
- finite production target and finite global consumption target.

Readers-writers:

- writer-preference policy.
- active reader/writer counters.
- waiting writer counter.
- read and write condition variables.

Dining philosophers:

- five philosophers.
- five chopstick mutexes.
- room limiter allowing at most four philosophers to compete for chopsticks at the same time.

## 7. Filesystem Design

Path: `basic/filesystem/`

The filesystem is an in-memory model:

- directory tree rooted at `/`.
- file nodes store size and block list.
- directory nodes store child nodes.
- bitmap tracks block allocation.
- block data is stored in memory.

Writes are overwrite-only. Before replacing file content, the implementation checks whether enough free blocks are available, so a failed write does not destroy the previous file content.

Path rules:

- absolute paths only.
- maximum path length enforced.
- component names have a fixed maximum length.
- allowed characters are letters, digits, `_`, `-`, and `.`.

## 8. Linux Observability Module

Path: `extension/oslab_monitor/`

Kernel module:

```text
kernel/oslab_monitor.c
```

Proc tree:

```text
/proc/oslab_monitor/
├── overview
├── tasks
└── pid
```

Permissions:

| Entry | Mode |
|---|---:|
| `overview` | `0444` |
| `tasks` | `0444` |
| `pid` | `0644` |

Implementation notes:

- proc entries use `seq_file` style output for stable long-text reads.
- process traversal uses kernel process iteration helpers.
- memory information is read through safe kernel interfaces.
- `mm == NULL` is handled for kernel threads or processes without an address space.
- all proc entries are removed during module exit.

## 9. `overview`

`overview` emits:

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

The implementation scans tasks and uses kernel memory-info helpers. Numeric fields are emitted as non-negative values.

## 10. `tasks`

`tasks` emits a stable table:

```text
PID     COMM            STATE   POLICY  PRIO  NICE  THREADS  RSS_KB  MIN_FLT  MAJ_FLT
```

The implementation traverses all processes and formats scheduling policy, priority, nice value, thread count, RSS, and fault fields. Missing or unavailable values are represented without breaking the column layout.

## 11. `pid`

`pid` supports:

- writing a positive target PID.
- reading selected PID details.
- reporting not found when the process no longer exists.
- bounded `copy_from_user` input handling.

Read fields:

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

## 12. `oslabctl`

Path: `extension/oslab_monitor/user/`

`oslabctl` forwards proc content without reformatting:

```bash
./oslabctl overview
./oslabctl tasks
sudo ./oslabctl pid 1
```

This avoids maintaining two output formats and keeps the CLI consistent with direct proc reads.

## 13. Validation

User-space validation:

```bash
bash tests/run_all.sh
```

Kernel-module validation:

```bash
cd extension/oslab_monitor
bash tests/test_oslab_monitor.sh
```

Validation scripts check exit codes, required fields, deterministic outputs, and cleanup behavior. The kernel-module script uses a trap to unload the module if a failure occurs mid-run.

## 14. Failure Modes

| Failure mode | Mitigation |
|---|---|
| malformed input | parse strictly, return non-zero, print `error:` |
| scheduler tie ambiguity | use documented arrival and input-order tie breakers |
| partition fragmentation after free | coalesce adjacent free partitions |
| failed filesystem write | check capacity before replacing old file data |
| synchronization deadlock | finite counters and deadlock-avoidance policy |
| long task list truncation | use `seq_file` style output |
| process without `mm` | check null address-space pointers |
| proc PID write overflow | bounded user buffer and integer parsing |
| stale proc entries after unload | remove entries in reverse creation order |

## 15. Design Decisions

### DD-001: Four Independent User-Space CLIs

Each OS mechanism has different input semantics, output semantics, and validation fixtures. Independent binaries keep module boundaries explicit and reduce hidden coupling.

### DD-002: No Cross-Module Common Library

The modules intentionally duplicate small parser and output helpers. This avoids introducing shared behavior that would couple independent mechanism models.

### DD-003: In-Memory Filesystem

The filesystem models directory, metadata, and block allocation behavior without persistent disk state. This keeps the mechanism focused and reproducible.

### DD-004: `/proc` + `seq_file` for Observability

`/proc` is directly inspectable from shell tools and fits process/memory observability. `seq_file` avoids fixed-buffer truncation for process listings.

### DD-005: Raw `oslabctl` Forwarding

The user-space wrapper reads and writes proc entries but does not reinterpret the kernel output. This keeps the proc interface as the single source of truth.
