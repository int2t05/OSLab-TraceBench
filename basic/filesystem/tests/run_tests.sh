#!/usr/bin/env bash
# 文件作用：验证文件系统模块的多级目录、覆盖写、读取、删除和错误命令。
# 设计原因：删除前后目录内容容易被人工误读，脚本用分段检查固定关键行为。

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

fs_output=$(./filesystem < tests/fs_commands.txt)
assert_contains "$fs_output" "hello_os"
assert_contains "$fs_output" "a.txt"
assert_contains "$fs_output" "total_blocks:"
assert_contains "$fs_output" "used_blocks:"
assert_contains "$fs_output" "free_blocks:"

after_delete=$(printf '%s\n' "$fs_output" | awk '
    /command: ls \/docs\/os/ { section += 1; next }
    section == 2 && /command:/ { exit }
    section == 2 { print }
')

if [[ "$after_delete" == *"a.txt"* ]]; then
    echo "error: deleted file still appears in second ls output" >&2
    echo "$after_delete" >&2
    exit 1
fi

set +e
invalid_output=$(./filesystem < tests/invalid.txt 2>&1)
invalid_status=$?
set -e

if [[ "$invalid_status" -eq 0 ]]; then
    echo "error: invalid filesystem input unexpectedly succeeded" >&2
    exit 1
fi
assert_contains "$invalid_output" "error:"

echo "filesystem tests passed"
