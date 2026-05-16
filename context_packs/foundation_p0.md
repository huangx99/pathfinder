# foundation P0 上下文包

## 1. P0 目标

实现 foundation 最小可编译模块，为后续 P1 (config + command) 提供稳定基础类型。

P0 完成后必须满足：

```text
backend CMake 可配置。
foundation target 可编译。
foundation tests 可运行。
Error / Result / StrongId / Version / Tick / Hash / RandomSeed / Serialization 约定已完成。
dev_notes/foundation.md 已记录完成状态。
没有提前实现上层模块。
```

## 2. 允许实现的类型

```text
ErrorCode
ErrorSeverity
ErrorCategory
ErrorDetail
Result<void>
Result<T>
StrongId<Tag>
StateVersion
Tick
DurationTicks
HashValue
RandomSeed
RandomDrawRecord
```

## 3. 禁止实现的上层模块

```text
ConfigLoader
RuleEngine
AgentRuntime
HTTP / WebSocket
H5 前端
CommandEnvelope
InteractionRule
FeedbackResolver
CognitionState
MemoryEntry
KnowledgeClaim
PropagationAttempt
TribeState
CivilizationState
```

## 4. 允许修改的目录

```text
backend/include/pathfinder/foundation/
backend/src/foundation/
backend/tests/unit/foundation/
backend/dev_notes/
context_packs/foundation_p0.md
```

## 5. 禁止修改的目录

```text
backend/include/pathfinder/engine/
backend/include/pathfinder/protocol/
backend/src/rules/
backend/src/pipeline/
backend/src/engine/
backend/src/protocol/
frontend/
doc/01-24
```

## 6. 验收命令

```text
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R foundation
```

## 7. 常见错误

```text
引入全局状态。
使用 std::random_device。
让 foundation 依赖上层模块。
没有测试。
ErrorCode 使用异常表达业务错误。
Result<T> 在错误状态调用 value() 行为未定义。
StrongId 允许空格。
HashValue 依赖内存地址。
```

## 8. 任务执行顺序

```text
TASK-P0-000: 创建 context pack (当前)
TASK-P0-001: 创建 backend CMake 骨架
TASK-P0-002: 创建最小测试入口
TASK-P0-003: 实现 ErrorCode / ErrorDetail
TASK-P0-004: 实现 Result<void>
TASK-P0-005: 实现 Result<T>
TASK-P0-006: 实现 StrongId 基础
TASK-P0-007: 实现 StrongId 格式校验和哈希
TASK-P0-008: 实现 StateVersion / Tick
TASK-P0-009: 实现 HashValue
TASK-P0-010: 实现 RandomSeed / RandomDrawRecord
TASK-P0-011: 实现 serialization.h 约定
TASK-P0-012: foundation 聚合检查
```

## 9. 前置文档索引

```text
doc/01_基础类型与通用约定.md: ID 约定、基础类型定义
doc/02_通用枚举设计.md: 枚举命名规则
doc/03_错误码与结果类型设计.md: ErrorCode、ErrorDetail、Result<T>
doc/22_测试策略设计.md: 测试原则
doc/23_工程实现路线图设计.md: 工程分层、依赖方向
doc/24_后端工程骨架设计.md: CMake 目标、文件结构、AI 任务卡
```
