# command P1 上下文包

## 1. P1 目标

在 P0 foundation 之上建立命令系统最小入口。

P1 完成后必须满足：

```text
CommandSource / CommandIntent / ActionTargetType / ActionTargetRole 枚举可用。
ActionTarget 可用。
ActionCommand 最小结构可用。
CommandEnvelope 可用。
CommandValidationReport 可用。
command 单元测试通过。
dev_notes/command.md 已记录任务历史。
没有提前实现规则引擎。
```

## 2. P1 边界

### 2.1 允许实现的类型

```text
CommandSource: player / ai / system / replay / test
CommandIntent: unknown / experiment / exploit / teach / avoid_risk / combine / migrate / fight
ActionTargetType: none / object / entity / location / region / knowledge / group
ActionTargetRole: primary / secondary / tool / material / receiver / destination
ActionTarget: target_type, target_id, role
ActionId: 基于 StrongId
ActorId: 基于 StrongId
ActionCommand: action_id, actor_id, targets, intent
CommandId: 基于 StrongId
CommandEnvelope: command_id, source, issued_tick, idempotency_key, correlation_id, payload
CommandValidationSeverity: warning / error
CommandValidationIssue: severity, ErrorCode, message, field_path
CommandValidationReport: issues, addIssue, hasErrors, issueCount
validateCommandEnvelopeBasic(envelope)
```

### 2.2 禁止实现

```text
RuleEngine 完整逻辑
RulePipeline 完整执行
InteractionRule 结算
AgentRuntime
HTTP / WebSocket
H5 前端
CommandRouter
CommandNormalizer 完整逻辑
CommandReplayRecord 完整逻辑
CommandParam 复杂参数
网络协议序列化
```

### 2.3 禁止行为

```text
ActionTarget 判断这个对象能不能吃
CommandEnvelope 直接扣血
在 ActionCommand 内判断 eat/use/combine 的具体规则
引入状态修改
实现 WorldObject 查询
实现 Entity 查询
```

## 3. 依赖约束

只能依赖 P0 foundation：

```text
ErrorCode
ErrorDetail
Result<T> / Result<void>
StrongId
isValidIdString
Tick
StateVersion
HashValue
RandomSeed
```

禁止依赖尚未实现的模块：

```text
engine
rules
pipeline
state
event
agent
protocol
frontend
config (P1 config 模块可选引用，但不强制)
```

## 4. 允许修改的目录

```text
backend/include/pathfinder/command/
backend/src/command/
backend/tests/unit/command/
backend/dev_notes/command.md
context_packs/command_p1.md
```

## 5. 禁止修改的目录

```text
backend/include/pathfinder/foundation/ 已有 API
backend/src/foundation/ 已有 API
backend/include/pathfinder/engine/
backend/include/pathfinder/rules/
backend/include/pathfinder/pipeline/
backend/include/pathfinder/agent/
backend/include/pathfinder/protocol/
frontend/
```

## 6. 测试要求

```text
每个枚举 toString 稳定
合法字符串可解析
非法字符串解析失败
新增枚举值时测试必须失败提醒补映射
合法 object target 通过
none target_type 失败
空 target_id 失败
非法 target_id 格式失败
不同 role 可以通过
合法 ActionCommand 通过
缺 action_id 失败
缺 actor_id 失败
空 targets 失败
非法 target 传播失败
intent unknown 允许通过
合法 envelope 通过
缺 command_id 失败
非法 source 失败
非法 payload 失败
空 idempotency_key 允许
过长 idempotency_key 失败
合法 envelope 生成空错误报告
非法 envelope 生成 error issue
warning 不算 error
issueCount 正确
```

## 7. 验收命令

```text
cmake --build build/backend
ctest --test-dir build/backend -R command --output-on-failure
```

## 8. 任务执行顺序

```text
TASK-P1-000: 创建 context pack (当前)
TASK-P1-001: 创建 config/command 目录与 CMake 空目标
TASK-P1-005: 实现命令基础枚举
TASK-P1-006: 实现 ActionTarget
TASK-P1-007: 实现 ActionCommand 最小结构
TASK-P1-008: 实现 CommandEnvelope
TASK-P1-009: 实现 CommandValidationReport
```

## 9. 前置文档索引

```text
doc/08_行为命令设计.md: ActionCommand, CommandEnvelope, CommandSource, CommandTarget 字段
doc/02_通用枚举设计.md: 枚举使用约定
doc/03_错误码与结果类型设计.md: Result / Error 约定
backend/include/pathfinder/foundation/error.h: ErrorCode, ErrorDetail
backend/include/pathfinder/foundation/result.h: Result<T>
backend/include/pathfinder/foundation/id.h: StrongId, ActionId, ActorId, CommandId
backend/include/pathfinder/foundation/time.h: Tick
```
