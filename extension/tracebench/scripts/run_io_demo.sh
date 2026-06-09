#!/usr/bin/env bash
#
# 文件作用：运行 TraceBench I/O profile 演示并生成 Markdown 报告。
# 设计原因：I/O 演示覆盖写入和 fsync 路径，可直接证明临时文件会在正常结束后清理。

set -euo pipefail

cd "$(dirname "$0")/.."

make
sudo ./tracebench run --profile io --duration 5 --sample-interval 1 --io-mb 64 --output output/io
./tracebench report --input output/io --output output/io/report.md
