# rules P3 上下文包

## 1. P3 目标

实现 rules 最小模块，承载 unknown fruit + eat 单条规则 resolver。

P3 完成后必须满足：

```text
EatObjectResolver 可解析 eat 命令。
UnknownFruitFixture 可提供稳定初始状态。
所有结构可测试。
不做完整 RuleEngine。
不做完整 RuleRegistry。
不做通用规则注册器。
不做复杂规则 DSL。
不做多对象反应系统。
不做 use/combine/collect/teach resolver。
```

## 2. P3 边界

### 2.1 允许实现

```text
EatObjectResolveInput: command / state
EatObjectResolveDraft: state_changes / events / cognition_evidence
EatObjectResolver::canResolve(command)
EatObjectResolver::resolve(input)
UnknownFruitFixture::createInitialState()
UnknownFruitFixture::createEatCommand()
```

### 2.2 禁止实现

```text
完整 RuleEngine。
完整 RuleRegistry。
完整 InteractionRule DSL。
use/combine/collect/teach 等其他行为 resolver。
复杂规则优先级系统。
ConfigLoader 读取真实配置文件。
多个对象定义。
```

## 3. 依赖约束

### 3.1 允许依赖

```text
foundation: ErrorCode / ErrorDetail / Result<T> / StrongId / Tick / StateVersion / HashValue
command: CommandEnvelope / ActionCommand / ActionTarget / ActionTargetType / CommandIntent
state: GameState / StateChange / StateChangeSet / StateChangeOperation / StateDomain
event: EventRecord / EventStream / EventType / EventVisibility / EventImportance / EventPayload
object: ObjectDefinition / WorldObject / ObjectStore / ObjectEdibleProfile / EdibleEffectKind
cognition: CognitionKey / CognitionEvidence / CognitionEffectKind / CognitionConfidence
```

### 3.2 禁止依赖

```text
engine
agent
protocol
frontend
config (读取真实配置)
memory
knowledge
propagation
```

## 4. 允许修改的目录

```text
backend/include/pathfinder/rules/
backend/src/rules/
backend/tests/unit/rules/
backend/tests/integration/p3/
backend/dev_notes/rules.md
```

## 5. 禁止修改的目录

```text
backend/include/pathfinder/engine/
backend/include/pathfinder/agent/
backend/include/pathfinder/protocol/
backend/include/pathfinder/config/ (除非编译必须)
frontend/
```

## 6. 测试要求

```text
eat 命令可以 resolve。
非 eat 命令 canResolve 为 false。
缺 actor 失败。
缺 object 失败。
已 consumed object 失败。
成功 resolve 产生至少 2 个 StateChange。
成功 resolve 产生 feedback event。
成功 resolve 产生 cognition evidence。
resolve 不修改输入 GameState。
fixture state validateBasic 通过。
fixture command validateBasic 通过。
初始 cognition 无 unknown_fruit + eat claim。
初始 object 未 consumed。
```

## 7. 验收命令

```bash
cmake --build build/backend
ctest --test-dir build/backend -R rules --output-on-failure
```

## 8. 常见错误

```text
把 use/combine/collect 写进 resolver。
让 resolver apply state。
让 resolver 直接写 EventStore。
通过 ConfigLoader 读取真实配置文件。
加入多个对象定义。
实现完整 RuleEngine。
创建 resolver registry。
```

## 9. 任务执行顺序

```text
TASK-P3-011: 实现 UnknownFruitFixture
TASK-P3-012: 实现 EatObjectResolver
```

## 10. 前置文档索引

```text
doc/09_交互规则设计.md: 核心交互边界
doc/10_反馈系统设计.md: 反馈与证据边界
doc/28_P3未知对象首条规则闭环任务卡设计.md: P3 任务卡
```

## 11. 设计关键约束

```text
Command 表达意图，不改状态。
Resolver 生成草稿，不直接改状态。
StateChangeGate 必须经过。
MinimalStateApplier 才能改 GameState。
EventRecord 记录事实。
Cognition 记录用途认知，不记录颜色形状。
P3 只允许 unknown fruit + eat 单条 resolver。
```
