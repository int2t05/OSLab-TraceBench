#!/usr/bin/env bash
#
# 文件作用：运行 TraceBench memory profile 演示并生成 Markdown 报告。
# 设计原因：内存压力参数固定为 PRDv2 默认值，避免课程 VM 中因过大参数触发不可控 OOM。

set -euo pipefail

cd "$(dirname "$0")/.."

make
sudo ./tracebench run --profile memory --duration 5 --sample-interval 1 --memory-mb 128 --output output/memory
./tracebench report --input output/memory --output output/memory/report.md
