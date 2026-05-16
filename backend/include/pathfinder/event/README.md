# event 模块

P2 阶段：EventRecord 最小结构 + EventStream / EventStore 最小追加能力。

## 边界

- 允许：EventType / EventVisibility / EventImportance / EventRecord / EventStream / InMemoryEventStore
- 禁止：异步 EventBus / 远程推送 / 数据库存储 / 事件驱动状态修改
- 禁止：复杂 EventPayload variant

## 依赖

- foundation: ErrorCode / ErrorDetail / Result<T> / StrongId / Tick / StateVersion
- state: StateChangeId

## 测试

```bash
ctest --test-dir build/backend -R event --output-on-failure
```
