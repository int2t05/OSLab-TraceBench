#!/usr/bin/env bash
#
# 文件作用：验证 TraceBench v2 P0 CLI 的基础行为。
# 设计原因：本测试放在模块自己的 tests 目录中，便于在 Ubuntu VM 中单独验收 TraceBench，
# 避免把需要 root 和 cgroup v2 的测试混入根目录基础模块测试。

set -euo pipefail

cd "$(dirname "$0")/.."

make

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

rm -rf output/test_nocg
./tracebench run --profile cpu --duration 1 --sample-interval 1 --output output/test_nocg --no-cgroup
test -f output/test_nocg/command.txt
test -f output/test_nocg/environment.txt
grep -q "profile: cpu" output/test_nocg/command.txt
grep -q "tracebench_version:" output/test_nocg/environment.txt

printf 'tracebench cli tests passed\n'
