# P46 Kimi 提交推送任务单

## 1. 任务

P46 已通过最终审核，现在请 Kimi 负责提交并推送代码。

不要修改代码，不要重新设计，不要扩大范围。

## 2. 必须提交的内容范围

提交 P46 相关代码、测试、设计和审核文档，包括但不限于：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/include/pathfinder/world_generation/
backend/src/world_generation/
backend/tests/unit/world_generation/
backend/tests/integration/world_generation/
backend/include/pathfinder/world_runtime/iworld_runtime.h
backend/include/pathfinder/world_runtime/world_grid_runtime.h
backend/include/pathfinder/world_runtime/world_runtime_types.h
backend/src/world_runtime/world_grid_runtime.cpp
doc/76_P46世界生成器与资源分布任务卡设计.md
doc/76A_P46世界生成器后续扩展风险头脑风暴.md
doc/76B_P46世界生成器测试文档.md
doc/76C_P46世界生成器修复文档.md
doc/review/P46_Kimi继续修复任务单.md
doc/review/P46_Kimi第二轮复审问题.md
doc/review/P46世界生成器与资源分布系统最终审核报告.md
```

如果工作区还有与 P44/P45 相关的未提交文件，不要擅自丢弃。若它们是 P46 前置文档且已经在工作区，应一起保留；如果无法判断，先 `git status --short` 后再决定。

## 3. 提交前必须确认

```bash
git status --short
```

确认没有临时构建产物、日志、/tmp 文件。

## 4. 提交命令

建议提交信息：

```text
feat: add P46 world generation runtime
```

## 5. 推送

提交成功后推送到当前分支：

```bash
git push origin main
```

## 6. 完成后回复

回复内容只需要：

```text
1. git commit hash
2. push 是否成功
3. git status 是否干净
```
