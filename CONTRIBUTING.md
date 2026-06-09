# 贡献指南

感谢关注 OSLab TraceBench。本仓库是 C/Linux 操作系统机制建模与运行态观测项目，贡献时请优先保证模块边界清晰、输出稳定和结果可复现。

## 开发原则

- 保持 `basic/`、`extension/oslab_monitor/` 和 `extension/tracebench/` 的边界独立。
- 不引入 GUI、Web 服务、宿主机调度器替换或 Linux 内核源码修改。
- 新增或修改命令输出时，同步更新 README、`docs/FEATURES.md`、需求文档和技术设计文档。
- 错误路径保持 `error:` 前缀和非零退出码。
- 不提交构建产物、内核模块产物、TraceBench 输出目录或临时运行文件。

## 本地验证

基础模块：

```bash
bash tests/run_all.sh
```

Linux 观测模块：

```bash
cd extension/oslab_monitor
bash tests/test_oslab_monitor.sh
```

TraceBench：

```bash
cd extension/tracebench
sudo bash tests/test_tracebench.sh
```

内核模块和 TraceBench 完整验证需要 Ubuntu 22.04 LTS 或 Ubuntu 24.04 LTS VM。

## 拉取请求要求

提交 PR 前请确认：

- 相关验证脚本已通过。
- 文档与实际命令、目录和输出字段一致。
- 没有提交 `*.o`、`*.ko`、`tracebench`、`oslabctl`、`output/` 等生成物。
- PR 描述中说明变更范围、验证命令和已知限制。

## 提交信息风格

建议使用简短的 conventional commit 风格：

```text
feat: add tracebench metric field
fix: handle empty scheduler input
docs: update tracebench usage
test: cover filesystem invalid command
```
