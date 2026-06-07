#!/usr/bin/env bash
# 文件作用：验证同步模块三个经典问题能在有限时间内结束并输出汇总字段。
# 设计原因：并发程序最容易出现无法退出或计数错误，timeout 可以把死锁和永久等待变成可验收失败。

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

pc_output=$(timeout 5s ./sync --problem producer_consumer --producers 2 --consumers 2 --buffer-size 4 --count 3)
assert_contains "$pc_output" "produced_total: 6"
assert_contains "$pc_output" "consumed_total: 6"
assert_contains "$pc_output" "buffer_final_size: 0"

rw_output=$(timeout 5s ./sync --problem readers_writers --readers 3 --writers 2 --count 2)
assert_contains "$rw_output" "read_total: 6"
assert_contains "$rw_output" "write_total: 4"
assert_contains "$rw_output" "final_shared_value: 4"

dining_output=$(timeout 5s ./sync --problem dining_philosophers --count 2)
assert_contains "$dining_output" "philosopher_0_eat_count: 2"
assert_contains "$dining_output" "philosopher_4_eat_count: 2"
assert_contains "$dining_output" "deadlock_detected: no"

set +e
invalid_output=$(./sync --problem producer_consumer --producers 0 2>&1)
invalid_status=$?
set -e

if [[ "$invalid_status" -eq 0 ]]; then
    echo "error: invalid sync arguments unexpectedly succeeded" >&2
    exit 1
fi
assert_contains "$invalid_output" "error:"

echo "sync tests passed"
