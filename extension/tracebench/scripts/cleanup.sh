#!/usr/bin/env bash
#
# 文件作用：执行 TraceBench 安全清理命令。
# 设计原因：cleanup 只清理 cgroup 命名空间和残留 I/O 临时文件，不删除 samples.csv、summary.txt 或报告。

set -euo pipefail

cd "$(dirname "$0")/.."

make
sudo ./tracebench cleanup
printf 'cleanup finished; samples.csv, summary.txt and report.md are preserved\n'
