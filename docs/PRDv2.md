# PRD v2: TraceBench Resource-Pressure Observability

## 1. Objective

TraceBench extends OSLab TraceBench with a user-space tool for controlled Linux resource-pressure generation and runtime sampling. The tool runs CPU, memory, and I/O workloads, records PSI and cgroup v2 counters, optionally samples `/proc/oslab_monitor/overview`, and emits reproducible text, CSV, summary, and Markdown artifacts.

TraceBench is implemented under `extension/tracebench/` and does not modify the basic OS models or the `oslab_monitor` kernel module.

## 2. Target Users

- Researchers comparing OS-level resource-pressure signals across workloads.
- Developers validating Linux PSI and cgroup v2 observability behavior in a VM.
- Reviewers reproducing benchmark runs from command-line inputs and generated artifacts.

## 3. Scope

Implemented baseline scope:

- `tracebench --help`
- `tracebench run`
- `tracebench report`
- `tracebench cleanup`
- CPU, memory, and I/O pressure profiles.
- cgroup v2 grouping and metric collection.
- PSI collection from `/proc/pressure/*`.
- Optional `oslab_monitor` comparison sampling.
- Stable `samples.csv` output.
- `command.txt`, `environment.txt`, and `summary.txt` metadata.
- Markdown report generation.
- Bash integration validation.

Out of scope:

- Linux kernel source modification.
- Host scheduler replacement.
- GUI or web UI.
- Mandatory external pressure tools such as `stress-ng` or `fio`.
- Mandatory tracing dependencies such as `perf` or `bpftrace`.
- sched_ext as a default workflow.

## 4. Runtime Environment

Reference environment:

```text
Ubuntu 22.04 LTS or Ubuntu 24.04 LTS VM
Linux kernel with cgroup v2
/proc/pressure/cpu
/proc/pressure/memory
/proc/pressure/io
gcc
make
bash
sudo/root for default cgroup mode
```

Full validation requires cgroup v2 and PSI support. `--no-cgroup` is available for low-permission exploration, but it does not produce full cgroup-backed results.

## 5. Command-Line Requirements

Executable:

```text
extension/tracebench/tracebench
```

Required commands:

```bash
./tracebench --help
sudo ./tracebench run --profile cpu --duration 5 --sample-interval 1 --output output/cpu
sudo ./tracebench run --profile memory --duration 5 --sample-interval 1 --output output/memory
sudo ./tracebench run --profile io --duration 5 --sample-interval 1 --output output/io
./tracebench report --input output/cpu --output output/cpu/report.md
sudo ./tracebench cleanup
```

Run parameters:

| Parameter | Requirement |
|---|---|
| `--profile` | required; one of `cpu`, `memory`, `io` |
| `--duration` | required positive integer seconds |
| `--sample-interval` | required positive integer seconds, not greater than duration |
| `--output` | required output directory for `run` |
| `--cpu-workers` | positive integer for CPU profile, default `2` |
| `--memory-mb` | positive integer for memory profile, default `128` |
| `--io-mb` | positive integer for I/O profile, default `64` |
| `--cgroup-name` | cgroup namespace name, default `oslab_tracebench` |
| `--no-cgroup` | disables cgroup creation and writes cgroup fields as `NA` |
| `--with-oslab-monitor` | requires `/proc/oslab_monitor/overview` to exist |

Report parameters:

| Parameter | Requirement |
|---|---|
| `--input` | directory containing `samples.csv` |
| `--output` | Markdown report path |

All errors must be written with an `error:` prefix and return a non-zero exit code.

## 6. Workload Requirements

CPU profile:

- Starts CPU-bound worker threads.
- Supports configurable worker count.
- Runs until the configured duration elapses.

Memory profile:

- Allocates the configured memory amount.
- Periodically touches allocated memory to keep pages active.
- Avoids setting cgroup memory limits by default.

I/O profile:

- Writes a fixed temporary file in the output directory.
- Calls `fsync` to expose storage pressure.
- Removes the temporary file on normal completion.

All workloads must be implemented in the project codebase and must be owned by the `tracebench` process tree.

## 7. Sampling Requirements

PSI:

- Read `/proc/pressure/cpu`.
- Read `/proc/pressure/memory`.
- Read `/proc/pressure/io`.
- Parse `some` and `full` lines when available.
- Record `avg10`, `avg60`, `avg300`, and `total`.

cgroup v2:

- Create `/sys/fs/cgroup/<cgroup-name>/<profile>/<run-id>/`.
- Move the workload child process into the run cgroup.
- Read `cpu.stat`.
- Read `memory.current`.
- Read `memory.events`.
- Remove empty cgroups created by the run.

`oslab_monitor` comparison:

- If `/proc/oslab_monitor/overview` exists, collect `total_tasks`, `running_tasks`, `sleeping_tasks`, `mem_free_kb`, and `mem_available_kb`.
- If absent in default mode, record `oslab_monitor_available=false`.
- If absent with `--with-oslab-monitor`, fail with `error:`.

## 8. Output Requirements

Each run writes the following files under the requested output directory:

```text
command.txt
environment.txt
samples.csv
summary.txt
```

`tracebench report` writes a Markdown report at the requested output path.

`samples.csv` must have a stable header. Required field groups:

- sample index and elapsed time.
- profile, duration, sample interval, and run id.
- cgroup enabled flag and path.
- CPU, memory, and I/O PSI fields.
- cgroup CPU and memory fields.
- `oslab_monitor` availability and selected counters.

`summary.txt` must include:

- profile.
- duration and sample interval.
- output path.
- cgroup status.
- sample count.
- references to generated files.

Markdown reports must include:

- run configuration.
- environment summary.
- `samples.csv` reference.
- PSI summary.
- cgroup summary.
- `oslab_monitor` comparison section.
- limitations and interpretation notes.

## 9. Cleanup Requirements

`tracebench cleanup` may remove only:

- cgroups under `/sys/fs/cgroup/<cgroup-name>/`.
- `tracebench_io.tmp` files under TraceBench output profile directories.

It must not delete CSV files, summaries, Markdown reports, user-created files, or arbitrary directories. If a cgroup still contains processes, cleanup must fail or skip that cgroup rather than killing unrelated processes.

## 10. Validation Requirements

The integration script is:

```bash
extension/tracebench/tests/test_tracebench.sh
```

It must validate:

- successful build.
- help output.
- parameter error paths.
- CPU profile run.
- memory profile run.
- I/O profile run.
- CSV header stability.
- CSV row field consistency.
- summary generation.
- Markdown report generation.
- cleanup behavior.
- `--with-oslab-monitor` failure when the proc interface is absent.

## 11. Acceptance Checklist

- [ ] `make` builds `tracebench`.
- [ ] `tracebench --help` shows `run`, `report`, and `cleanup`.
- [ ] CPU profile runs and exits.
- [ ] Memory profile runs and exits.
- [ ] I/O profile runs and exits.
- [ ] Each profile writes `samples.csv`.
- [ ] CSV includes PSI fields.
- [ ] CSV includes cgroup fields.
- [ ] CSV includes `oslab_monitor` fields.
- [ ] `tracebench report` generates Markdown.
- [ ] `tracebench cleanup` preserves generated CSV, summary, and report files.
- [ ] Invalid parameters return non-zero and print `error:`.
- [ ] `sudo bash tests/test_tracebench.sh` passes on the reference VM.
