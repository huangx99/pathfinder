# pipeline 模块

P2 阶段：RulePipeline 空壳 + PipelineContext / PipelineResult。

## 边界

- 允许：PipelineStage / PipelineStatus / PipelineStep / PipelineContext / PipelineResult / RulePipeline 空执行器
- 禁止：完整 RuleEngine / ActionResolver / InteractionResolver
- 禁止：eat/use/combine 特判

## 依赖

- foundation: ErrorCode / ErrorDetail / Result<T> / StrongId / Tick / StateVersion / RandomSeed
- config: ConfigPackage / ConfigVersion
- command: CommandEnvelope / ActionCommand / ActionTarget
- state: GameState / StateChangeSet
- event: EventStream

## 测试

```bash
ctest --test-dir build/backend -R pipeline --output-on-failure
```
