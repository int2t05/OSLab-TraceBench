# OSLab TraceBench

OSLab TraceBench is a C/Linux research prototype for operating-system mechanism modeling and Linux runtime observability. It combines deterministic user-space models for core OS mechanisms with kernel-level `/proc` instrumentation and a resource-pressure tracing tool for reproducible CPU, memory, and I/O evaluation on Linux.

The repository is organized as a formal research codebase: source code, build files, reproducible input data, validation scripts, and technical documentation are kept; draft notes, prompt records, and generated runtime artifacts are intentionally excluded.

## Scope

The project has three independently usable parts:

- `basic/`: standalone C implementations for processor scheduling, memory management, process synchronization, and an in-memory filesystem model.
- `extension/oslab_monitor/`: a Linux kernel module and user-space CLI for observing process and memory state through `/proc/oslab_monitor/`.
- `extension/tracebench/`: a user-space resource-pressure and sampling tool that records PSI, cgroup v2, and optional `oslab_monitor` signals into CSV, summary text, and Markdown reports.

The project does not modify the Linux kernel source tree and does not replace the host scheduler. All kernel-facing work is limited to a loadable module and `/proc` interfaces.

## Research Use Cases

- Compare deterministic scheduling policies with stable inputs and reproducible output tables.
- Analyze dynamic partition allocation and page replacement behavior under fixed reference strings.
- Observe synchronization behavior across producer-consumer, readers-writers, and dining-philosophers workloads.
- Inspect real Linux process, scheduling, and memory fields through a purpose-built `/proc` module.
- Generate CPU, memory, and I/O pressure while sampling PSI and cgroup v2 counters in a controlled Ubuntu VM.
- Produce reproducible CSV and Markdown artifacts for system-behavior analysis.

## Environment

User-space modules:

```text
Linux
gcc
make
bash
pthread
```

Kernel and TraceBench modules:

```text
Ubuntu 22.04 LTS or Ubuntu 24.04 LTS VM
build-essential
linux-headers-$(uname -r)
cgroup v2
/proc/pressure/cpu
/proc/pressure/memory
/proc/pressure/io
sudo/root for kernel-module loading and default TraceBench cgroup mode
```

WSL2 is not treated as the reference environment for kernel-module or full cgroup validation.

## Quick Start

Run the deterministic user-space validation suite:

```bash
bash tests/run_all.sh
```

Build and inspect the Linux observability module in an Ubuntu VM:

```bash
cd extension/oslab_monitor
bash tests/test_oslab_monitor.sh
```

Build and validate TraceBench in an Ubuntu VM:

```bash
cd extension/tracebench
sudo bash tests/test_tracebench.sh
```

## Basic Modules

Processor scheduling:

```bash
cd basic/scheduler
make
./scheduler --algorithm fcfs < tests/sample.txt
./scheduler --algorithm sjf < tests/sample.txt
./scheduler --algorithm rr < tests/sample.txt
./scheduler --algorithm priority < tests/sample.txt
make clean
```

Memory management:

```bash
cd basic/memory
make
./memory --mode partition --algorithm ff < tests/partition.txt
./memory --mode partition --algorithm bf < tests/partition.txt
./memory --mode paging --algorithm fifo < tests/pages.txt
./memory --mode paging --algorithm lru < tests/pages.txt
make clean
```

Process synchronization:

```bash
cd basic/sync
make
./sync --problem producer_consumer --producers 2 --consumers 2 --buffer-size 4 --count 10
./sync --problem readers_writers --readers 3 --writers 2 --count 5
./sync --problem dining_philosophers --count 3
make clean
```

In-memory filesystem:

```bash
cd basic/filesystem
make
./filesystem < tests/fs_commands.txt
make clean
```

## Linux Observability Module

Build and load:

```bash
cd extension/oslab_monitor/kernel
make
sudo insmod oslab_monitor.ko
```

Read exported interfaces:

```bash
cat /proc/oslab_monitor/overview
cat /proc/oslab_monitor/tasks
echo 1 | sudo tee /proc/oslab_monitor/pid
cat /proc/oslab_monitor/pid
```

Use the CLI wrapper:

```bash
cd extension/oslab_monitor/user
make
./oslabctl --help
./oslabctl overview
./oslabctl tasks
sudo ./oslabctl pid 1
```

Unload:

```bash
cd extension/oslab_monitor/kernel
sudo rmmod oslab_monitor
make clean
```

## TraceBench

Build and inspect the CLI:

```bash
cd extension/tracebench
make
./tracebench --help
```

Run controlled pressure profiles:

```bash
sudo ./tracebench run --profile cpu --duration 3 --sample-interval 1 --cpu-workers 2 --output output/cpu_run
sudo ./tracebench run --profile memory --duration 3 --sample-interval 1 --memory-mb 64 --output output/memory_run
sudo ./tracebench run --profile io --duration 3 --sample-interval 1 --io-mb 16 --output output/io_run
```

Generate a Markdown report and clean runtime state:

```bash
./tracebench report --input output/cpu_run --output output/cpu_run/report.md
sudo ./tracebench cleanup
```

Each `tracebench run` writes:

```text
command.txt
environment.txt
samples.csv
summary.txt
```

## Repository Layout

```text
basic/                         OS mechanism models
basic/scheduler/               FCFS, SJF, RR, and priority scheduling
basic/memory/                  dynamic partition and page replacement models
basic/sync/                    pthread-based synchronization workloads
basic/filesystem/              in-memory directory and block-allocation model
extension/oslab_monitor/        Linux kernel module and /proc user CLI
extension/tracebench/           resource-pressure sampling tool
tests/run_all.sh                user-space validation entry point
docs/PRD.md                     v1 requirements and scope
docs/PRDv2.md                   TraceBench requirements and scope
docs/TECH.md                    v1 technical design
docs/TECHv2.md                  TraceBench technical design
docs/FEATURES.md                implemented capability index
```

## Verified Reference Environment

```text
Ubuntu 24.04.2 LTS
Linux 6.11.0-17-generic
gcc 13.3.0
GNU Make 4.3
```

Validated commands:

```bash
bash tests/run_all.sh
cd extension/oslab_monitor
bash tests/test_oslab_monitor.sh
cd extension/tracebench
sudo bash tests/test_tracebench.sh
```
