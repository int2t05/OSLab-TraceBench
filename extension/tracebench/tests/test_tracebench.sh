#!/usr/bin/env bash
#
# 文件作用：验证 TraceBench v2 P0 CLI 的基础行为。
# 设计原因：本测试放在模块自己的 tests 目录中，便于在 Ubuntu VM 中单独验收 TraceBench，
# 避免把需要 root 和 cgroup v2 的测试混入根目录基础模块测试。

set -euo pipefail

cd "$(dirname "$0")/.."

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

check_error "invalid profile" \
    ./tracebench run --profile bad --duration 1 --sample-interval 1 --output output/bad

check_error "invalid duration" \
    ./tracebench run --profile cpu --duration 0 --sample-interval 1 --output output/bad

check_error "sample interval greater than duration" \
    ./tracebench run --profile cpu --duration 1 --sample-interval 2 --output output/bad

check_error "invalid memory size" \
    ./tracebench run --profile memory --duration 1 --sample-interval 1 --memory-mb 0 --output output/bad_memory

rm -rf output/test_nocg
./tracebench run --profile cpu --duration 1 --sample-interval 1 --output output/test_nocg --no-cgroup
test -f output/test_nocg/command.txt
test -f output/test_nocg/environment.txt
test -f output/test_nocg/samples.csv
grep -q "profile: cpu" output/test_nocg/command.txt
grep -q "tracebench_version:" output/test_nocg/environment.txt
grep -q "cpu_some_avg10" output/test_nocg/samples.csv
grep -q "memory_some_avg10" output/test_nocg/samples.csv
grep -q "io_some_avg10" output/test_nocg/samples.csv

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

rm -rf output/test_cpu
start_sec="$(date +%s)"
./tracebench run --profile cpu --duration 3 --sample-interval 1 --cpu-workers 2 --output output/test_cpu
end_sec="$(date +%s)"
elapsed_sec=$((end_sec - start_sec))
test "$elapsed_sec" -ge 2
test "$elapsed_sec" -le 6
test -f output/test_cpu/command.txt
test -f output/test_cpu/environment.txt

rm -rf output/test_memory
start_sec="$(date +%s)"
./tracebench run --profile memory --duration 3 --sample-interval 1 --memory-mb 64 --output output/test_memory
end_sec="$(date +%s)"
elapsed_sec=$((end_sec - start_sec))
test "$elapsed_sec" -ge 2
test "$elapsed_sec" -le 6
test -f output/test_memory/command.txt
test -f output/test_memory/environment.txt

printf 'tracebench cli tests passed\n'
