#!/usr/bin/env bash
# 文件作用：验证内存模块的动态分区、页面置换 oracle 和错误输入。
# 设计原因：FF/BF/FIFO/LRU 的输出需要可重复检查，脚本用于固定回归验证关键字段。

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

ff_output=$(./memory --mode partition --algorithm ff < tests/partition.txt)
assert_contains "$ff_output" "algorithm: ff"
assert_contains "$ff_output" "allocated_partitions:"
assert_contains "$ff_output" "free_partitions:"
assert_contains "$ff_output" "P4 130 50"

bf_output=$(./memory --mode partition --algorithm bf < tests/partition.txt)
assert_contains "$bf_output" "algorithm: bf"
assert_contains "$bf_output" "allocated_partitions:"
assert_contains "$bf_output" "free_partitions:"
assert_contains "$bf_output" "P4 130 50"

fifo_output=$(./memory --mode paging --algorithm fifo < tests/pages.txt)
assert_contains "$fifo_output" "algorithm: fifo"
assert_contains "$fifo_output" "page_faults: 10"
assert_contains "$fifo_output" "fault_rate: 76.92%"

lru_output=$(./memory --mode paging --algorithm lru < tests/pages.txt)
assert_contains "$lru_output" "algorithm: lru"
assert_contains "$lru_output" "page_faults: 9"
assert_contains "$lru_output" "fault_rate: 69.23%"

set +e
invalid_output=$(./memory --mode paging --algorithm fifo < tests/invalid.txt 2>&1)
invalid_status=$?
set -e

if [[ "$invalid_status" -eq 0 ]]; then
    echo "error: invalid input unexpectedly succeeded" >&2
    exit 1
fi
assert_contains "$invalid_output" "error:"

echo "memory tests passed"
