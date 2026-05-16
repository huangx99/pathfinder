# replay 模块开发笔记

## 模块职责

记录命令执行和随机过程，支持重放校验。

## 公开类型

- ReplayRecordStatus: Unknown / Recorded / Replayed / Mismatch
- ReplayRecordId: StrongId tag for record IDs
- CommandReplayRecord: 一次命令执行的复盘记录
- CommandReplayLog: 内存命令复盘日志
- RandomReplayEntryId: StrongId tag for random entry IDs
- RandomReplayEntry: 一次随机抽取记录
- RandomReplayLog: 内存随机复盘日志
- ReplayCompareStatus: Unknown / Match / Mismatch
- ReplayCompareReport: 重放对比报告
- ReplayInput: 重放输入 (初始状态 + 命令日志 + 随机日志)
- ReplayResult: 重放结果 (对比报告 + PipelineResult)
- ReplayRunner: 重放运行器，通过 RulePipeline 重放

## 禁止依赖

- engine
- agent
- frontend
- HTTP / WebSocket
- 文件系统

## 已实现

- CommandReplayRecord / CommandReplayLog (P4)
- RandomReplayEntry / RandomReplayLog (P4)
- ReplayRunner::replayOne 单命令重放 (P4)
- CommandReplayRecord::validateBasic 完整校验 (record_id/command_id 格式、command.validateBasic、command_id 一致性、hash 非空)
- RandomReplayEntry::validateBasic 完整校验 (entry_id 格式、draw_index 负数、min/max/result 范围)
- CommandReplayLog::append / RandomReplayLog::append 调用 validateBasic 拒绝非法记录
- ReplayRunner::replayOne 校验 initial_state / command_log / random_log，检查 seed 一致性
- draw_index 类型为 int64_t，负数校验真实可测
- ReplayResult 增加 replay_state，P4 集成测试比较第一次执行与重放执行的事件类型序列 / hunger / consumed / cognition claim
- 30 个单元测试通过

## 未实现

- 完整 SaveManager
- 文件持久化
- 多命令重放
- 复杂随机规则

## 测试命令

```bash
ctest --test-dir build/backend -R replay --output-on-failure
```

## 架构约束

- ReplayRunner 必须通过 RulePipeline::execute 重放，不能直接调用 Resolver
- ReplayRunner 不能直接修改 GameState（使用副本重放）
- CommandReplayLog 不修改 GameState
- RandomReplayLog 只记录随机过程，不生成玩法结果
