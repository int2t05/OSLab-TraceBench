#!/usr/bin/env bash
# 文件作用：验证调度模块的固定样例、空闲时间段和错误输入。
# 设计原因：课程验收依赖稳定输出，脚本集中保存 oracle，避免手工比对遗漏。

set -euo pipefail

assert_contains() {
    local output="$1"
    local expected="$2"
    if [[ "$output" != *"$expected"* ]]; then
        echo "error: expected output to contain: $expected" >&2
        echo "$output" >&2
        exit 1
    fi
}

make

fcfs_output=$(./scheduler --algorithm fcfs < tests/sample.txt)
assert_contains "$fcfs_output" "P1[0,5] P2[5,8] P3[8,16] P4[16,22]"
assert_contains "$fcfs_output" "average_waiting: 5.75"
assert_contains "$fcfs_output" "average_turnaround: 11.25"

sjf_output=$(./scheduler --algorithm sjf < tests/sample.txt)
assert_contains "$sjf_output" "P1[0,5] P2[5,8] P4[8,14] P3[14,22]"
assert_contains "$sjf_output" "average_waiting: 5.25"
assert_contains "$sjf_output" "average_turnaround: 10.75"

priority_output=$(./scheduler --algorithm priority < tests/sample.txt)
assert_contains "$priority_output" "P1[0,5] P2[5,8] P4[8,14] P3[14,22]"

rr_output=$(./scheduler --algorithm rr < tests/sample.txt)
assert_contains "$rr_output" "P1[0,2] P2[2,4] P3[4,6] P1[6,8] P4[8,10] P2[10,11] P3[11,13] P1[13,14] P4[14,16] P3[16,18] P4[18,20] P3[20,22]"
assert_contains "$rr_output" "average_waiting: 9.75"
assert_contains "$rr_output" "average_turnaround: 15.25"

idle_output=$(./scheduler --algorithm fcfs < tests/idle_case.txt)
assert_contains "$idle_output" "IDLE[0,3]"
assert_contains "$idle_output" "IDLE[5,7]"

set +e
invalid_output=$(./scheduler --algorithm rr < tests/invalid.txt 2>&1)
invalid_status=$?
set -e

if [[ "$invalid_status" -eq 0 ]]; then
    echo "error: invalid input unexpectedly succeeded" >&2
    exit 1
fi
assert_contains "$invalid_output" "error:"

echo "scheduler tests passed"
