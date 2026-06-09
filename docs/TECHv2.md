# TECH v2: TraceBench Technical Design

## 1. Design Goals

TraceBench is a single C command-line program that generates controlled Linux resource pressure and samples runtime signals. It is implemented under `extension/tracebench/` and is independent of the basic OS models and the `oslab_monitor` kernel module.

Design goals:

- Minimal dependencies: C, Makefile, Bash, and Linux system interfaces.
- Stable text and CSV outputs.
- Explicit privilege behavior for cgroup v2.
- Safe cleanup boundaries.
- Reproducible command, environment, sample, summary, and report artifacts.

## 2. Architecture

```text
tracebench
├── args.c       command-line parsing and validation
├── cgroup.c     cgroup v2 creation, process assignment, sampling, cleanup
├── workload.c   CPU, memory, and I/O workload execution
├── sampler.c    PSI, cgroup, and oslab_monitor sampling
├── report.c     summary.txt and Markdown report generation
├── util.c       filesystem, time, string, and error helpers
└── main.c       command dispatch
```

The parent process parses arguments, creates output files, manages cgroup state, starts a workload child process, samples periodically, writes CSV rows, generates summary data, and performs run-scoped cleanup.

The workload child process waits for the parent to finish cgroup assignment before beginning pressure generation. This ensures sampled cgroup metrics correspond to the current run.

## 3. Directory Layout

```text
extension/tracebench/
├── Makefile
├── include/
│   └── tracebench.h
├── src/
│   ├── main.c
│   ├── args.c
│   ├── cgroup.c
│   ├── workload.c
│   ├── sampler.c
│   ├── report.c
│   └── util.c
└── tests/
    └── test_tracebench.sh
```

Generated runtime outputs are written under user-specified output directories and are not committed.

## 4. Build

`extension/tracebench/Makefile` builds a single executable:

```bash
cd extension/tracebench
make
```

Compiler settings:

```text
gcc
-Wall -Wextra -std=c11
-D_POSIX_C_SOURCE=200809L
-pthread
```

`make clean` removes only build products. It does not remove generated run data.

## 5. Commands

```bash
./tracebench --help
sudo ./tracebench run --profile cpu --duration 5 --sample-interval 1 --output output/cpu
sudo ./tracebench run --profile memory --duration 5 --sample-interval 1 --output output/memory
sudo ./tracebench run --profile io --duration 5 --sample-interval 1 --output output/io
./tracebench report --input output/cpu --output output/cpu/report.md
sudo ./tracebench cleanup
```

Default cgroup mode requires root. `--no-cgroup` disables cgroup creation and writes cgroup fields as `NA`.

## 6. Core Data Model

Main configuration:

```c
typedef enum {
    TB_CMD_HELP,
    TB_CMD_RUN,
    TB_CMD_REPORT,
    TB_CMD_CLEANUP
} TbCommand;

typedef enum {
    TB_PROFILE_CPU,
    TB_PROFILE_MEMORY,
    TB_PROFILE_IO
} TbProfile;

typedef struct {
    TbCommand command;
    TbProfile profile;
    int duration_sec;
    int sample_interval_sec;
    int cpu_workers;
    int memory_mb;
    int io_mb;
    int no_cgroup;
    int with_oslab_monitor;
    char cgroup_name[64];
    char output_path[512];
    char input_path[512];
    char report_output_path[512];
} TbConfig;
```

cgroup state:

```c
typedef struct {
    int enabled;
    char root_path[512];
    char profile_path[512];
    char run_path[512];
    char run_id[64];
} TbCgroup;
```

PSI snapshot:

```c
typedef struct {
    int present;
    double avg10;
    double avg60;
    double avg300;
    unsigned long long total;
} TbPsiLine;
```

## 7. Run Flow

`tracebench run` follows this sequence:

1. Parse and validate command-line arguments.
2. Validate privileges and required kernel interfaces.
3. Create the output directory.
4. Write `command.txt`.
5. Write `environment.txt`.
6. Create cgroup paths unless `--no-cgroup` is set.
7. Fork the workload child.
8. Move the workload child into the run cgroup.
9. Signal the child to start pressure generation.
10. Sample PSI, cgroup, and optional `oslab_monitor` data until duration elapses.
11. Stop and reap the child.
12. Write `summary.txt`.
13. Remove empty run cgroups and temporary I/O files.

## 8. Workloads

CPU:

- Child process creates `--cpu-workers` pthread workers.
- Workers run a CPU-bound loop until the parent-controlled duration ends.

Memory:

- Child process allocates `--memory-mb`.
- Pages are touched repeatedly to keep memory pressure visible.

I/O:

- Child process writes `<output>/tracebench_io.tmp`.
- Writes are followed by `fsync`.
- The temporary file is removed on normal completion.

## 9. Sampling

PSI sources:

```text
/proc/pressure/cpu
/proc/pressure/memory
/proc/pressure/io
```

cgroup sources:

```text
cpu.stat
memory.current
memory.events
```

Optional comparison source:

```text
/proc/oslab_monitor/overview
```

All samples are emitted to `samples.csv` with a fixed header. Missing optional values are written as `NA`; absent `oslab_monitor` data is represented by `oslab_monitor_available=false`.

## 10. Report Generation

`tracebench report` reads:

```text
<input>/samples.csv
```

It writes a Markdown report containing:

- run configuration.
- environment reference.
- `samples.csv` path.
- PSI summary.
- cgroup summary.
- `oslab_monitor` comparison summary.
- limitations.

Summary statistics use simple `first`, `last`, `delta`, and `max` calculations. No plotting or external report generator is required.

## 11. Cleanup Boundaries

`tracebench cleanup` is intentionally narrow. It may remove:

```text
/sys/fs/cgroup/<cgroup-name>/
extension/tracebench/output/*/tracebench_io.tmp
```

It must not delete:

- `samples.csv`
- `summary.txt`
- Markdown reports
- arbitrary user files
- output directories requested by the user

If a cgroup contains processes, cleanup skips or fails that cgroup rather than killing unrelated processes.

## 12. Error Handling

Every failing path prints `error:` and returns a non-zero exit code.

Examples:

```text
error: --profile must be one of cpu, memory, io
error: cgroup mode requires root; rerun with sudo or use --no-cgroup to collect without cgroup metrics
error: /proc/pressure/cpu not found
error: samples.csv not found under output/cpu
```

## 13. Validation

The integration script is:

```bash
cd extension/tracebench
sudo bash tests/test_tracebench.sh
```

It validates:

- build.
- help output.
- invalid argument handling.
- CPU, memory, and I/O runs.
- cgroup-enabled and no-cgroup modes.
- PSI, cgroup, and `oslab_monitor` CSV fields.
- CSV field-count consistency.
- summary and Markdown report generation.
- cleanup behavior.

## 14. Design Decisions

### DD-001: Keep TraceBench Independent

TraceBench lives in `extension/tracebench/` and does not modify `basic/` or `extension/oslab_monitor/`. It may read `oslab_monitor` output but does not require the module to be loaded.

### DD-002: Require Explicit Root for cgroup Mode

Default cgroup mode requires root. The tool does not invoke `sudo` internally. This avoids hidden privilege escalation and prevents accidental partial data collection.

### DD-003: Use Parent Sampling and Child Workload

The parent process owns sampling and cleanup. The child process owns pressure generation. A synchronization pipe ensures pressure starts only after cgroup assignment.

### DD-004: Keep Output Text-Based

The tool produces text, CSV, and Markdown. This keeps the project dependency-free and easy to inspect from a terminal.

### DD-005: Limit Cleanup Scope

Cleanup is constrained to TraceBench-owned cgroups and exact temporary-file names. Generated research artifacts are preserved by default.
