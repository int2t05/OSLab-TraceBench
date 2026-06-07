#!/usr/bin/env bash
# 文件作用：作为基础四模块的一键测试入口。
# 设计原因：扩展模块需要 root 和可加载内核模块环境，根测试只覆盖基础 CLI，避免在普通环境误加载内核模块。

set -euo pipefail

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)

run_module_tests() {
    local module_dir="$1"
    local module_name="$2"

    echo "== running ${module_name} tests =="
    (
        cd "$ROOT_DIR/$module_dir"
        bash tests/run_tests.sh
        make clean
    )
}

run_module_tests "basic/scheduler" "scheduler"
run_module_tests "basic/memory" "memory"
run_module_tests "basic/filesystem" "filesystem"
run_module_tests "basic/sync" "sync"

echo "basic module tests passed"
echo "extension tests require Ubuntu VM with root permissions:"
echo "  cd extension/oslab_monitor"
echo "  bash tests/test_oslab_monitor.sh"
