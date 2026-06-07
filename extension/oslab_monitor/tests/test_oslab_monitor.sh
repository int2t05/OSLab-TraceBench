#!/usr/bin/env bash
# 文件作用：在 Ubuntu VM 中集成验证 oslab_monitor 内核模块、/proc 接口和 oslabctl。
# 设计原因：内核模块测试可能中途失败，trap 尽量卸载模块，避免污染后续验收环境。

set -euo pipefail

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
MODULE_NAME="oslab_monitor"

cleanup() {
    if lsmod | awk '{print $1}' | grep -qx "$MODULE_NAME"; then
        sudo rmmod "$MODULE_NAME" || true
    fi
}

assert_contains() {
    local output="$1"
    local expected="$2"
    if [[ "$output" != *"$expected"* ]]; then
        echo "error: expected output to contain: $expected" >&2
        echo "$output" >&2
        exit 1
    fi
}

trap cleanup EXIT

cd "$ROOT_DIR/kernel"
make
sudo insmod oslab_monitor.ko

if [[ ! -d /proc/oslab_monitor ]]; then
    echo "error: /proc/oslab_monitor missing after insmod" >&2
    exit 1
fi

overview_output=$(cat /proc/oslab_monitor/overview)
assert_contains "$overview_output" "module:"
assert_contains "$overview_output" "kernel:"
assert_contains "$overview_output" "total_tasks:"
assert_contains "$overview_output" "mem_total_kb:"

tasks_output=$(cat /proc/oslab_monitor/tasks | head -5)
assert_contains "$tasks_output" "PID     COMM            STATE   POLICY  PRIO  NICE  THREADS  RSS_KB  MIN_FLT  MAJ_FLT"

echo 1 | sudo tee /proc/oslab_monitor/pid >/dev/null
pid_output=$(cat /proc/oslab_monitor/pid)
assert_contains "$pid_output" "pid:"
assert_contains "$pid_output" "comm:"

cd "$ROOT_DIR/user"
make
ctl_overview=$(./oslabctl overview)
assert_contains "$ctl_overview" "module:"
ctl_tasks=$(./oslabctl tasks | head -5)
assert_contains "$ctl_tasks" "PID     COMM"
ctl_pid=$(sudo ./oslabctl pid 1)
assert_contains "$ctl_pid" "pid:"
assert_contains "$ctl_pid" "comm:"

cd "$ROOT_DIR/kernel"
sudo rmmod "$MODULE_NAME"

if [[ -d /proc/oslab_monitor ]]; then
    echo "error: /proc/oslab_monitor still exists after rmmod" >&2
    exit 1
fi

trap - EXIT
echo "oslab monitor integration tests passed"
