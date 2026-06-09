#!/usr/bin/env bash
#
# 文件作用：运行 TraceBench CPU profile 演示并生成 Markdown 报告。
# 设计原因：演示脚本固定课程报告常用参数，便于在 Ubuntu VM 中复现实验输出和截图。

set -euo pipefail

cd "$(dirname "$0")/.."

make
sudo ./tracebench run --profile cpu --duration 5 --sample-interval 1 --cpu-workers 2 --output output/cpu
./tracebench report --input output/cpu --output output/cpu/report.md
