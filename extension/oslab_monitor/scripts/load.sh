#!/usr/bin/env bash
# 文件作用：编译并加载 oslab_monitor 内核模块。
# 设计原因：加载流程需要 root 权限和环境检查，脚本能让演示步骤稳定复现。

set -euo pipefail

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)

cd "$ROOT_DIR/kernel"
make
sudo insmod oslab_monitor.ko

if [[ ! -d /proc/oslab_monitor ]]; then
    echo "error: /proc/oslab_monitor missing after insmod" >&2
    exit 1
fi

lsmod | grep '^oslab_monitor'
ls /proc/oslab_monitor
