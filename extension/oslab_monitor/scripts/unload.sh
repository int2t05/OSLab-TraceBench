#!/usr/bin/env bash
# 文件作用：卸载 oslab_monitor 内核模块并检查 /proc 节点已清理。
# 设计原因：卸载验证是内核模块可靠性检查重点，脚本统一输出 dmesg 尾部便于运行记录留存。

set -euo pipefail

sudo rmmod oslab_monitor

if [[ -d /proc/oslab_monitor ]]; then
    echo "error: /proc/oslab_monitor still exists after rmmod" >&2
    exit 1
fi

dmesg | tail
