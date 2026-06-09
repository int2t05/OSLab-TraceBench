#!/usr/bin/env bash
#
# 文件作用：验证 TraceBench v2 P0 CLI 的基础行为。
# 设计原因：本测试放在模块自己的 tests 目录中，便于在 Ubuntu VM 中单独验收 TraceBench，
# 避免把需要 root 和 cgroup v2 的测试混入根目录基础模块测试。

set -euo pipefail

cd "$(dirname "$0")/.."

cleanup() {
    # 使用 trap 是为了测试中途失败时仍尽量清理 cgroup 和残留 I/O 临时文件，避免污染后续验收。
    if [ -x ./tracebench ]; then
        ./tracebench cleanup >/dev/null 2>&1 || true
    fi
}

trap cleanup EXIT

if [ "$(id -u)" -eq 0 ]; then
    echo "running as root"
else
    echo "error: tracebench P0 integration test requires sudo/root" >&2
    exit 1
fi

mount | grep -q cgroup2
test -d /sys/fs/cgroup

make clean
make
test -x ./tracebench

bash -n scripts/run_cpu_demo.sh
bash -n scripts/run_memory_demo.sh
bash -n scripts/run_io_demo.sh
bash -n scripts/cleanup.sh

require_readable() {
    local path="$1"

    if [ ! -r "$path" ]; then
        printf 'error: %s is required for TraceBench P0 PSI sampling\n' "$path" >&2
        exit 1
    fi
}

require_readable /proc/pressure/cpu
require_readable /proc/pressure/memory
require_readable /proc/pressure/io
oslab_monitor_present=0
if [ -r /proc/oslab_monitor/overview ]; then
    oslab_monitor_present=1
fi

help_output="$(./tracebench --help)"
grep -q "run" <<<"$help_output"
grep -q "report" <<<"$help_output"
grep -q "cleanup" <<<"$help_output"

check_error() {
    local name="$1"
    shift
    local output
    local status

    set +e
    output="$("$@" 2>&1)"
    status=$?
    set -e

    if [ "$status" -eq 0 ]; then
        printf 'error: %s unexpectedly succeeded\n' "$name" >&2
        exit 1
    fi

    grep -q "error:" <<<"$output"
}

check_csv_shape() {
    local path="$1"

    awk -F',' 'NR==1 {n=NF} NR>1 && NF!=n {exit 1}' "$path"
}

check_csv_header() {
    local path="$1"
    local expected

    expected="sample_index,elapsed_ms,profile,duration_sec,sample_interval_sec,run_id,cgroup_enabled,cgroup_path"
    expected="$expected,cpu_some_avg10,cpu_some_avg60,cpu_some_avg300,cpu_some_total"
    expected="$expected,cpu_full_avg10,cpu_full_avg60,cpu_full_avg300,cpu_full_total"
    expected="$expected,memory_some_avg10,memory_some_avg60,memory_some_avg300,memory_some_total"
    expected="$expected,memory_full_avg10,memory_full_avg60,memory_full_avg300,memory_full_total"
    expected="$expected,io_some_avg10,io_some_avg60,io_some_avg300,io_some_total"
    expected="$expected,io_full_avg10,io_full_avg60,io_full_avg300,io_full_total"
    expected="$expected,cgroup_cpu_usage_usec,cgroup_cpu_user_usec,cgroup_cpu_system_usec"
    expected="$expected,cgroup_cpu_nr_periods,cgroup_cpu_nr_throttled,cgroup_cpu_throttled_usec"
    expected="$expected,cgroup_memory_current,cgroup_memory_events_low,cgroup_memory_events_high"
    expected="$expected,cgroup_memory_events_max,cgroup_memory_events_oom"
    expected="$expected,cgroup_memory_events_oom_kill,cgroup_memory_events_oom_group_kill"
    expected="$expected,oslab_monitor_available,oslab_total_tasks,oslab_running_tasks"
    expected="$expected,oslab_sleeping_tasks,oslab_mem_free_kb,oslab_mem_available_kb"

    test "$(head -n 1 "$path")" = "$expected"
}

check_min_rows() {
    local path="$1"
    local min_rows="$2"
    local rows

    rows=$(($(wc -l < "$path") - 1))
    test "$rows" -ge "$min_rows"
}

check_error "invalid profile" \
    ./tracebench run --profile bad --duration 1 --sample-interval 1 --output output/bad

check_error "invalid duration" \
    ./tracebench run --profile cpu --duration 0 --sample-interval 1 --output output/bad

check_error "sample interval greater than duration" \
    ./tracebench run --profile cpu --duration 1 --sample-interval 2 --output output/bad

check_error "invalid memory size" \
    ./tracebench run --profile memory --duration 1 --sample-interval 1 --memory-mb 0 --output output/bad_memory

check_error "invalid io size" \
    ./tracebench run --profile io --duration 1 --sample-interval 1 --io-mb 0 --output output/bad_io

if [ "$oslab_monitor_present" -eq 0 ]; then
    check_error "with oslab_monitor but module missing" \
        ./tracebench run --profile cpu --duration 1 --sample-interval 1 --output output/with_om --with-oslab-monitor
fi

check_error "report missing samples" \
    ./tracebench report --input output/missing_report_input --output output/missing_report.md

rm -rf output/test_nocg
./tracebench run --profile cpu --duration 2 --sample-interval 1 --cpu-workers 1 --output output/test_nocg --no-cgroup
test -f output/test_nocg/command.txt
test -f output/test_nocg/environment.txt
test -f output/test_nocg/samples.csv
test -f output/test_nocg/summary.txt
grep -q "profile: cpu" output/test_nocg/command.txt
grep -q "tracebench_version:" output/test_nocg/environment.txt
grep -q "no-cgroup" output/test_nocg/summary.txt
grep -q "cpu_some_avg10" output/test_nocg/samples.csv
grep -q "memory_some_avg10" output/test_nocg/samples.csv
grep -q "io_some_avg10" output/test_nocg/samples.csv
grep -q "cgroup_enabled" output/test_nocg/samples.csv
grep -q "false" output/test_nocg/samples.csv
grep -q "NA" output/test_nocg/samples.csv
check_csv_header output/test_nocg/samples.csv
check_csv_shape output/test_nocg/samples.csv

rm -rf output/test_cgroup
./tracebench run --profile cpu --duration 1 --sample-interval 1 --output output/test_cgroup
test -f output/test_cgroup/command.txt
test -f output/test_cgroup/environment.txt
test -f output/test_cgroup/samples.csv
grep -q "cgroup_enabled" output/test_cgroup/samples.csv
grep -q "cgroup_memory_current" output/test_cgroup/samples.csv

if [ -n "${SUDO_USER:-}" ] && [ "$SUDO_USER" != "root" ]; then
    set +e
    sudo -u "$SUDO_USER" ./tracebench run --profile cpu --duration 1 --sample-interval 1 --output output/non_root_cpu >output/non_root.out 2>&1
    status=$?
    set -e
    test "$status" -ne 0
    grep -q "error:" output/non_root.out
else
    printf 'warning: skipping non-root cgroup failure check because SUDO_USER is unavailable\n' >&2
fi

if [ -n "${SUDO_USER:-}" ] && [ "$SUDO_USER" != "root" ]; then
    set +e
    sudo -u "$SUDO_USER" ./tracebench cleanup >output/cleanup_non_root.out 2>&1
    status=$?
    set -e
    test "$status" -ne 0
    grep -q "error:" output/cleanup_non_root.out
else
    printf 'warning: skipping non-root cleanup failure check because SUDO_USER is unavailable\n' >&2
fi

rm -rf output/test_cpu
start_sec="$(date +%s)"
./tracebench run --profile cpu --duration 3 --sample-interval 1 --cpu-workers 2 --output output/test_cpu
end_sec="$(date +%s)"
elapsed_sec=$((end_sec - start_sec))
test "$elapsed_sec" -ge 2
test "$elapsed_sec" -le 6
test -f output/test_cpu/command.txt
test -f output/test_cpu/environment.txt
test -f output/test_cpu/samples.csv
grep -q "cpu_some_avg10" output/test_cpu/samples.csv
grep -q "cgroup_cpu_usage_usec" output/test_cpu/samples.csv
grep -q "oslab_monitor_available" output/test_cpu/samples.csv
if [ "$oslab_monitor_present" -eq 0 ]; then
    grep -q "false" output/test_cpu/samples.csv
else
    grep -q "oslab_total_tasks" output/test_cpu/samples.csv
fi
check_csv_header output/test_cpu/samples.csv
check_min_rows output/test_cpu/samples.csv 3
check_csv_shape output/test_cpu/samples.csv
test -f output/test_cpu/summary.txt
grep -q "profile" output/test_cpu/summary.txt
grep -q "samples.csv" output/test_cpu/summary.txt
./tracebench report --input output/test_cpu --output output/test_cpu/report.md
test -f output/test_cpu/report.md
grep -q "PSI" output/test_cpu/report.md
grep -q "cgroup" output/test_cpu/report.md
grep -q "oslab_monitor" output/test_cpu/report.md
grep -q "samples.csv" output/test_cpu/report.md

rm -rf output/test_memory
start_sec="$(date +%s)"
./tracebench run --profile memory --duration 3 --sample-interval 1 --memory-mb 64 --output output/test_memory
end_sec="$(date +%s)"
elapsed_sec=$((end_sec - start_sec))
test "$elapsed_sec" -ge 2
test "$elapsed_sec" -le 6
test -f output/test_memory/command.txt
test -f output/test_memory/environment.txt
test -f output/test_memory/samples.csv
test -f output/test_memory/summary.txt
grep -q "memory_some_avg10" output/test_memory/samples.csv
grep -q "cgroup_memory_current" output/test_memory/samples.csv
check_csv_header output/test_memory/samples.csv
check_min_rows output/test_memory/samples.csv 3
check_csv_shape output/test_memory/samples.csv

rm -rf output/test_io
start_sec="$(date +%s)"
./tracebench run --profile io --duration 3 --sample-interval 1 --io-mb 16 --output output/test_io
end_sec="$(date +%s)"
elapsed_sec=$((end_sec - start_sec))
test "$elapsed_sec" -ge 2
test "$elapsed_sec" -le 8
test -f output/test_io/command.txt
test -f output/test_io/environment.txt
test ! -f output/test_io/tracebench_io.tmp
test -f output/test_io/samples.csv
test -f output/test_io/summary.txt
grep -q "io_some_avg10" output/test_io/samples.csv
check_csv_header output/test_io/samples.csv
check_min_rows output/test_io/samples.csv 3
check_csv_shape output/test_io/samples.csv

./tracebench cleanup
test -f output/test_cpu/samples.csv
test -f output/test_cpu/summary.txt
test -f output/test_cpu/report.md
test ! -f output/test_io/tracebench_io.tmp

printf 'tracebench cli tests passed\n'
