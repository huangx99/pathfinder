# pipeline P2 上下文包

## 1. P2 目标

实现 pipeline 最小模块，为 RulePipeline 空壳执行器提供承载结构。

P2 完成后必须满足：

```text
PipelineStage / PipelineStatus 枚举可用。
PipelineContext 可构造、可校验。
PipelineResult 可构造、可校验。
RulePipeline 空执行器可返回成功结果。
所有结构可测试。
不执行真实游戏规则。
不实现完整 RuleEngine。
```

## 2. P2 边界

### 2.1 允许实现

```text
PipelineStage: AcceptCommand / ValidateCommand / PrepareContext / ResolveRules / ValidateStateChanges / ApplyStateChanges / BuildEvents / EmitEvents / Finish
PipelineStatus: NotStarted / Running / Succeeded / Failed
PipelineStep: stage / order / name
defaultPipelineSteps()
PipelineContext: command / state_metadata / config_version / random_seed / correlation_id
PipelineResult: status / executed_steps / state_changes / events / errors
RulePipeline::execute(context) -> Result<PipelineResult>
```

### 2.2 禁止实现

```text
完整 RuleEngine
ActionResolver
InteractionResolver
FeedbackResolver 真实逻辑
CognitionResolver 真实逻辑
任何 eat/use/combine 特判
复杂 Pipeline 插件注册器
StageRegistry
PipelineDefinitionValidator
DraftCollector
PipelineTransaction
```

## 3. 依赖约束

### 3.1 允许依赖

```text
foundation: ErrorCode / ErrorDetail / Result<T> / StrongId / Tick / StateVersion / RandomSeed
config: ConfigPackage / ConfigVersion
command: CommandEnvelope / ActionCommand / ActionTarget
state: GameState / StateChangeSet (P2 state 模块)
event: EventStream (P2 event 模块)
```

### 3.2 禁止依赖

```text
engine
rules
agent
protocol
frontend
replay
```

## 4. 允许修改的目录

```text
backend/include/pathfinder/pipeline/
backend/src/pipeline/
backend/tests/unit/pipeline/
backend/dev_notes/pipeline.md
```

## 5. 禁止修改的目录

```text
backend/include/pathfinder/state/ (除非编译必须)
backend/include/pathfinder/event/ (除非编译必须)
backend/include/pathfinder/engine/
backend/include/pathfinder/rules/
backend/include/pathfinder/agent/
backend/include/pathfinder/protocol/
frontend/
```

## 6. 测试要求

```text
toString/fromString 稳定。
defaultPipelineSteps 非空。
defaultPipelineSteps order 严格递增。
defaultPipelineSteps 包含 Finish。
非法字符串解析失败。
合法 context 通过。
非法 command 失败。
非法 state 失败。
context 构造不修改 GameState。
correlation_id 可为空。
空成功结果合法。
带错误的结果 hasErrors 为 true。
executed_steps 顺序错误失败。
非法 StateChangeSet 失败。
非法 EventStream 失败。
合法 context 执行成功。
非法 context 执行失败。
执行结果 status 为 Succeeded。
执行结果 state_changes 为空。
执行结果 events 为空。
executed_steps 顺序正确。
输入 GameState 未改变。
没有调用任何 rules/engine 模块。
```

## 7. 验收命令

```bash
cmake --build build/backend
ctest --test-dir build/backend -R pipeline --output-on-failure
rg -n "eat|use|combine|InteractionRule|Resolver|RuleEngine" backend/include/pathfinder/pipeline backend/src/pipeline backend/tests/unit/pipeline
```

## 8. 常见错误

```text
在 PipelineContext 中放 resolver 列表。
在 PipelineContext 中放 H5/HTTP 信息。
把 PipelineResult 设计成前端协议对象。
在 PipelineResult 里直接携带完整 GameState 拷贝。
实现 eat/use/combine 特判。
创建 engine 目录。
让 Pipeline 直接调用 rules 模块。
PipelineStage 缺少 toString/fromString。
defaultPipelineSteps 顺序不递增。
```

## 9. 任务执行顺序

```text
TASK-P2-009: 实现 PipelineStage / PipelineStep
TASK-P2-010: 实现 PipelineContext
TASK-P2-011: 实现 PipelineResult
TASK-P2-012: 实现 RulePipeline 空执行器
```

## 10. 前置文档索引

```text
doc/02_通用枚举设计.md: 枚举命名规则
doc/17_规则管线设计.md: 管线顺序原则
doc/27_P2状态事件管线任务卡设计.md: P2 任务卡
```
