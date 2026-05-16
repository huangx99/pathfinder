# event P2 上下文包

## 1. P2 目标

实现 event 最小模块，为 EventRecord、EventStream 和 InMemoryEventStore 提供承载结构。

P2 完成后必须满足：

```text
EventType / EventVisibility / EventImportance 枚举可用。
EventRecord 可构造、可校验。
EventStream 可聚合事件。
InMemoryEventStore 可追加事件。
所有结构可测试。
不实现异步 EventBus。
不实现远程推送。
不实现数据库存储。
```

## 2. P2 边界

### 2.1 允许实现

```text
EventType: Unknown / CommandAccepted / CommandRejected / StateChanged / PipelineStarted / PipelineFinished / Debug
EventVisibility: Hidden / PlayerVisible / TribeVisible / DebugOnly / ReplayOnly
EventImportance: Low / Normal / High / Critical
EventPayload: payload_type / message_key / debug_text
EventRecord: event_id / event_type / command_id(optional) / correlation_id(optional) / tick / state_version / state_change_ids / source_id(optional) / target_ids / payload / visibility / importance
EventStream: events / addEvent / empty / size / validateBasic
InMemoryEventStore: append(stream) / append(record) / size / allEvents / clear
EventValidationReport
```

### 2.2 禁止实现

```text
异步 EventBus
远程推送
前端协议事件流
数据库事件存储
事件驱动状态修改
复杂 EventPayload variant
EventBuilder / EventFactory (P2 暂不需要)
EventSubscriber
EventProjection
EventReplay
```

## 3. 依赖约束

### 3.1 允许依赖

```text
foundation: ErrorCode / ErrorDetail / Result<T> / StrongId / Tick / StateVersion / HashValue
state: StateChangeId (P2 state 模块)
```

### 3.2 禁止依赖

```text
engine
rules
agent
protocol
frontend
replay
config (除非编译必须)
command (除非编译必须)
pipeline (P2 pipeline 模块)
```

## 4. 允许修改的目录

```text
backend/include/pathfinder/event/
backend/src/event/
backend/tests/unit/event/
backend/dev_notes/event.md
```

## 5. 禁止修改的目录

```text
backend/include/pathfinder/state/ (除非编译必须)
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
Unknown 只允许作为默认占位，不应作为正式事件类型通过 EventRecord 校验。
合法 EventRecord 通过。
缺 event_id 失败。
event_type Unknown 失败。
payload_type 空失败。
可以没有 command_id。
可以绑定多个 state_change_id。
空 EventStream 合法。
添加合法事件后 size 增加。
非法事件进入 stream 后 validateBasic 失败。
EventStore append 合法事件成功。
EventStore append 非法事件失败。
EventStore 保持追加顺序。
clear 清空存储。
```

## 7. 验收命令

```bash
cmake --build build/backend
ctest --test-dir build/backend -R event --output-on-failure
```

## 8. 常见错误

```text
让 EventRecord 修改状态。
EventStore 修改 GameState。
实现异步 EventBus。
实现数据库存储。
实现前端事件推送。
EventType 使用 Unknown 作为正式事件。
EventPayload 做成巨大 variant。
缺少 toString/fromString。
```

## 9. 任务执行顺序

```text
TASK-P2-006: 实现 Event 基础枚举 (EventType / EventVisibility / EventImportance)
TASK-P2-007: 实现 EventRecord
TASK-P2-008: 实现 EventStream / InMemoryEventStore
```

## 10. 前置文档索引

```text
doc/02_通用枚举设计.md: 枚举命名规则
doc/04_事件系统设计.md: EventRecord / Visibility / Importance 章节
doc/27_P2状态事件管线任务卡设计.md: P2 任务卡
```
