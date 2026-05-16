# state P2 上下文包

## 1. P2 目标

实现 state 最小模块，为 GameState 最小快照和 StateChange 草稿提供承载结构。

P2 完成后必须满足：

```text
GameState 有最小 metadata。
StateChange / StateChangeSet 可构造、可校验。
StateChangeGate 可做最小校验。
所有结构可测试。
不执行真实游戏规则。
没有提前实现完整对象库、认知、族群状态。
```

## 2. P2 边界

### 2.1 允许实现

```text
GameStateId
StateSnapshotId
StateChangeOperation: Create / Update / Delete / Append / Remove / NoOp
StateChangeStatus: Draft / Validated / Applied / Rejected
StateDomain: Unknown / World / Object / Entity / Region / Knowledge / Tribe / Civilization / System
GameStateMetadata: state_id / state_version / current_tick / config_version / state_hash
GameState: metadata
StatePath: domain / target_id / field_path
StateChange: change_id / operation / domain / target_id / field_path / before_hash / after_hash / status / reason
StateChangeSet: changes / addChange / empty / size / validateBasic
StateChangeValidationIssue
StateChangeValidationReport
StateChangeGate::validate
```

### 2.2 禁止实现

```text
完整 WorldObject 存储
完整 Entity 属性系统
完整资源系统
完整认知/记忆/知识状态
复杂 diff/patch 引擎
真实状态提交 apply
eat/use/combine 规则
```

## 3. 依赖约束

### 3.1 允许依赖

```text
foundation: ErrorCode / ErrorDetail / Result<T> / StrongId / Tick / StateVersion / HashValue
config: ConfigPackage / ConfigVersion
command: CommandEnvelope / ActionCommand / ActionTarget
```

### 3.2 禁止依赖

```text
engine
rules
agent
protocol
frontend
replay
event (P2 event 模块)
pipeline (P2 pipeline 模块)
```

## 4. 允许修改的目录

```text
backend/include/pathfinder/state/
backend/src/state/
backend/tests/unit/state/
backend/dev_notes/state.md
backend/dev_notes/foundation.md
backend/include/pathfinder/foundation/id.h (新增 GameStateId / StateSnapshotId)
```

## 5. 禁止修改的目录

```text
backend/include/pathfinder/event/
backend/include/pathfinder/pipeline/
backend/include/pathfinder/engine/
backend/include/pathfinder/rules/
backend/include/pathfinder/agent/
backend/include/pathfinder/protocol/
frontend/
```

## 6. 测试要求

```text
每个枚举 toString 稳定。
合法字符串可解析。
非法字符串解析失败。
GameStateId / StateSnapshotId 可构造、比较、哈希。
合法 GameState 通过校验。
空 state_id 失败。
非法 state_id 格式失败。
空 StateChangeSet 合法。
合法 StateChange 可加入集合。
缺 change_id 失败。
operation 为 NoOp 时必须有 reason。
field_path 为空时 Update/Delete/Append/Remove 失败。
重复 change_id 失败。
合法空变更集通过 StateChangeGate。
非法 GameState 失败。
非法 StateChange 失败。
重复写同一字段失败。
validate 不修改输入 GameState。
```

## 7. 验收命令

```bash
cmake --build build/backend
ctest --test-dir build/backend -R state --output-on-failure
ctest --test-dir build/backend -R foundation --output-on-failure
```

## 8. 常见错误

```text
引入完整对象 map。
引入 eat/use/combine 规则字段。
让 StateChangeGate 执行真实 apply。
StateChange 存储任意 JSON patch。
GameState 依赖 event/pipeline 模块。
枚举缺少 toString/fromString。
Result<T> 失败时未返回错误。
```

## 9. 任务执行顺序

```text
TASK-P2-002: 实现 State 基础类型 (GameStateId / StateSnapshotId / 枚举)
TASK-P2-003: 实现 GameState 最小快照
TASK-P2-004: 实现 StateChange / StateChangeSet
TASK-P2-005: 实现 StateChangeGate 最小校验
```

## 10. 前置文档索引

```text
doc/02_通用枚举设计.md: 枚举命名规则
doc/06_世界对象设计.md: WorldObject 设计 (仅参考，不实现)
doc/17_规则管线设计.md: StateChangeGate 章节
doc/27_P2状态事件管线任务卡设计.md: P2 任务卡
```
