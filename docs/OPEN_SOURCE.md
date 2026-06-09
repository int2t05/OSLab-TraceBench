# 开源上线信息

本文档记录 OSLab TraceBench 的公开仓库定位、GitHub About 建议、主题标签和发布前检查清单。

## 项目定位

OSLab TraceBench 是一个 C/Linux 研究型开源项目，用于操作系统机制建模、Linux `/proc` 运行态观测和 CPU/内存/I/O 资源压力采样。

## GitHub 简介建议

```text
C/Linux 操作系统机制模型、Linux /proc 观测模块与 TraceBench 资源压力采样工具，用于可复现的 OS 行为分析。
```

## 推荐主题标签

```text
operating-systems
linux
c
systems-programming
kernel-module
procfs
cgroup-v2
psi
benchmarking
observability
scheduler
memory-management
filesystem
pthreads
oslab
```

选择理由：

- `operating-systems`、`scheduler`、`memory-management`、`filesystem` 对应基础机制模型。
- `linux`、`kernel-module`、`procfs` 对应内核观测扩展。
- `cgroup-v2`、`psi`、`benchmarking`、`observability` 对应 TraceBench。
- `c`、`systems-programming`、`pthreads` 对应技术栈。

## 开源策略

- 许可证：木兰宽松许可证第 2 版（`MulanPSL-2.0`）。该许可证是宽松型许可证，包含版权许可、专利许可和明确的无商标许可条款，适合 C/Linux 系统工具开源。
- 受众：系统编程学习者、操作系统机制研究者、Linux 观测工具开发者。
- 发布重点：强调无需额外压力工具即可生成资源压力，并能输出 CSV、摘要和 Markdown 报告。
- 社区入口：优先通过 GitHub Issues 收集缺陷、复现问题和功能建议。

## 上线检查清单

- [ ] `main` 分支已包含最新代码和文档。
- [ ] README 为中文，并包含实际使用场景、快速开始、验证命令和许可证。
- [ ] `LICENSE` 存在且 SPDX 与 README 一致。
- [ ] `CONTRIBUTING.md`、`SECURITY.md`、`CHANGELOG.md` 和 `CITATION.cff` 存在。
- [ ] Issue/PR 模板存在。
- [ ] 基础验证脚本通过。
- [ ] Ubuntu VM 中内核模块和 TraceBench 验证通过。
- [ ] 仓库没有提交构建产物、运行输出或过程性草稿。
