#!/usr/bin/env bash
# 文件作用：演示 /proc/oslab_monitor 和 oslabctl 的主要输出。
# 设计原因：课程报告需要可复制的运行结果，脚本按固定顺序展示关键字段。

set -euo pipefail

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)

echo "== overview =="
cat /proc/oslab_monitor/overview

echo "== tasks =="
head /proc/oslab_monitor/tasks

echo "== pid =="
echo 1 | sudo tee /proc/oslab_monitor/pid >/dev/null
cat /proc/oslab_monitor/pid

echo "== oslabctl =="
cd "$ROOT_DIR/user"
make
./oslabctl overview
tasks_output=$(mktemp)
./oslabctl tasks > "$tasks_output"
head "$tasks_output"
rm -f "$tasks_output"
sudo ./oslabctl pid 1
