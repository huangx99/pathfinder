# replay_p4 Context Pack

## P4 目标

在 P3 已通过的 unknown_fruit + eat 规则闭环之上，实现最小复盘记录与重放校验。

## 允许实现

- CommandReplayRecord / CommandReplayLog
- RandomReplayEntry / RandomReplayLog
- ReplayCompareReport / ReplayInput / ReplayResult
- ReplayRunner::replayOne (通过 RulePipeline 重放)

## 禁止实现

- 完整 SaveManager
- 真实文件落盘格式
- 数据库
- HTTP / WebSocket
- H5 前端
- AgentRuntime
- 复杂随机规则
- 多命令剧情回放

## 关键约束

- ReplayRunner 必须通过 RulePipeline::execute 重放，不能直接调用 EatObjectResolver
- ReplayRunner 不能直接修改 GameState
- CommandReplayLog 不修改 GameState
- RandomReplayLog 不生成玩法结果，只记录随机过程
- P4 只做内存日志，不做文件持久化

## 依赖模块

- foundation: StrongId, Result, ErrorDetail, HashValue, RandomSeed, StateVersion
- command: CommandEnvelope, ActionCommand
- state: GameState, StateChangeSet
- event: EventStream, EventRecord
- pipeline: PipelineContext, PipelineResult, RulePipeline
- rules: UnknownFruitFixture (仅用于测试)

## 验收命令

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R replay --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

## 测试清单

- CommandReplayRecord 合法通过
- 缺 record_id 失败
- 缺 command_id 失败
- 重复 record_id append 失败
- findByCommandId 可找到记录
- RandomReplayEntry 合法通过
- 缺 entry_id 失败
- draw_index < 0 失败
- min_value > max_value 失败
- result_value 超范围失败
- P3 unknown_fruit eat 记录可重放成功
- 同样输入得到 Match
- 篡改 output_hash 得到 Mismatch
- 空 CommandReplayLog 返回错误
