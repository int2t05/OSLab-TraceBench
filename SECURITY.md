# 安全策略

## 支持范围

当前仓库以 `main` 分支作为公开维护分支。安全问题、崩溃、越界读写、权限边界错误和内核模块卸载清理问题都应优先报告。

## 报告方式

请通过 GitHub Security Advisory 或私有渠道报告安全问题。公开 Issue 适合普通缺陷，不适合披露可利用细节。

报告时请包含：

- 受影响的模块和提交版本。
- 复现环境，包括内核版本和发行版版本。
- 复现步骤。
- 预期行为和实际行为。
- 可能的影响范围。

## 权限边界说明

- `oslab_monitor` 需要 root 加载内核模块。
- `/proc/oslab_monitor/pid` 设计为 `0644`。
- TraceBench 默认 cgroup 模式需要 root。
- `tracebench cleanup` 只应清理 TraceBench 自有 cgroup 和精确命名的临时文件。
