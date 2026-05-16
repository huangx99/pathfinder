# pipeline P3 上下文包

## 1. P3 目标

扩展 RulePipeline 以执行 P3 unknown fruit + eat 单条规则闭环。

P3 完成后必须满足：

```text
RulePipeline 可在 P3 场景下调用 EatObjectResolver。
PipelineContext 可承载 GameState 引用或 P3 最小状态输入。
PipelineResult 返回真实 StateChangeSet / EventStream。
完整链路: resolve -> StateChangeGate -> apply -> events -> cognition。
所有结构可测试。
不实现完整 RuleEngine。
不实现动态 resolver 注册系统。
不实现协议输出。
不实现 Agent 决策。
```

## 2. P3 边界

### 2.1 允许实现

```text
RulePipeline P3 执行分支 (eat 命令)。
PipelineContext 承载 GameState 引用或可变 GameState。
PipelineResult 返回真实 StateChangeSet / EventStream。
调用 EatObjectResolver。
调用 StateChangeGate::validate。
调用 MinimalStateApplier::apply。
调用 CognitionUpdater::applyEvidence。
把事件加入 PipelineResult.events。
非 eat 命令返回不支持或空失败。
```

### 2.2 禁止实现

```text
完整 RuleEngine。
动态 resolver 注册系统。
协议输出。
Agent 决策。
完整 resolver 注册系统。
eat 以外的 resolver 调用。
```

## 3. 依赖约束

### 3.1 允许依赖

```text
foundation: ErrorCode / ErrorDetail / Result<T> / StrongId / Tick / StateVersion / HashValue / RandomSeed
config: ConfigVersion
command: CommandEnvelope / ActionCommand
state: GameState / StateChange / StateChangeSet / StateChangeGate / MinimalStateApplier
event: EventRecord / EventStream / EventType / EventVisibility / EventImportance
pipeline: PipelineContext / PipelineResult / RulePipeline / PipelineStage / PipelineStatus
object: ObjectStore / ObjectDefinition / WorldObject
cognition: CognitionState / CognitionUpdater / CognitionEvidence
rules: EatObjectResolver / EatObjectResolveInput / EatObjectResolveDraft
```

### 3.2 禁止依赖

```text
engine
agent
protocol
frontend
```

## 4. 允许修改的目录

```text
backend/include/pathfinder/pipeline/context.h
backend/include/pathfinder/pipeline/result.h
backend/include/pathfinder/pipeline/rule_pipeline.h
backend/src/pipeline/context.cpp
backend/src/pipeline/result.cpp
backend/src/pipeline/rule_pipeline.cpp
backend/tests/unit/pipeline/rule_pipeline_test.cpp
backend/dev_notes/pipeline.md
```

## 5. 禁止修改的目录

```text
backend/include/pathfinder/engine/
backend/include/pathfinder/agent/
backend/include/pathfinder/protocol/
frontend/
```

## 6. 测试要求

```text
合法 unknown fruit eat 命令执行成功。
PipelineResult.status == Succeeded。
state_changes 非空。
events 非空。
actor hunger 从 80 到 60。
object consumed。
cognition claim 被创建。
非 eat 命令返回 unsupported 或 failed，不修改 state。
```

## 7. 验收命令

```bash
cmake --build build/backend
ctest --test-dir build/backend -R pipeline --output-on-failure
ctest --test-dir build/backend -R rules --output-on-failure
ctest --test-dir build/backend -R p3 --output-on-failure
```

## 8. 常见错误

```text
引入完整 RuleEngine。
创建 resolver registry。
把规则结果绕过 StateChangeGate。
让 Pipeline 直接修改 GameState 而不经过 MinimalStateApplier。
在 Pipeline 中写大量玩法 if/else。
```

## 9. 任务执行顺序

```text
TASK-P3-007: 扩展 GameState 承载 P3 最小状态
TASK-P3-008: 扩展 StateChange 支持 P3 StateValue
TASK-P3-009: 实现 MinimalStateApplier
TASK-P3-010: 扩展 EventType / EventRecord
TASK-P3-013: 接入 RulePipeline P3 执行
TASK-P3-014: 编写 unknown fruit 集成测试
```

## 10. 前置文档索引

```text
doc/17_规则管线设计.md: 管线顺序原则
doc/28_P3未知对象首条规则闭环任务卡设计.md: P3 任务卡
```

## 11. 设计关键约束

```text
Command 不直接改状态。
Resolver 不绕过 StateChangeGate。
StateChange 先校验再 apply。
EventRecord 记录事实。
Cognition 不保存真实规则，只保存主体认知。
Pipeline 负责编排，不把所有逻辑写成巨大 if/else。
```
