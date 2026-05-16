# state 模块

P2 阶段：GameState 最小快照 + StateChange 草稿。

## 边界

- 允许：GameState / StateChange / StateChangeSet / StateChangeGate 最小校验
- 禁止：完整 WorldObject / Entity / 资源 / 认知 / 记忆 / 知识状态
- 禁止：eat/use/combine 规则
- 禁止：真实状态提交 apply

## 依赖

- foundation: ErrorCode / ErrorDetail / Result<T> / StrongId / Tick / StateVersion / HashValue
- config: ConfigPackage / ConfigVersion
- command: CommandEnvelope / ActionCommand / ActionTarget

## 测试

```bash
ctest --test-dir build/backend -R state --output-on-failure
```
