# P9 Agent复盘锁定与训练样本前置复审报告

审核时间：2026-05-17  
审核对象：P9 Agent复盘锁定与训练样本前置修复版  
审核结论：通过，允许进入 P10。

## 1. 复审范围

本次复审依据：

```text
doc/34_P9Agent复盘锁定与训练样本前置任务卡设计.md
context_packs/agent_p9.md
上次 P9 审核报告中的阻塞项和建议项
```

重点复查：

```text
P9 集成测试是否移除直接 GameState 使用。
TrainingSampleDraftBuilder 的 phase_keys / reason_keys 映射是否修正。
AgentReplayLockSet::validateBasic 是否校验 source_hash。
AgentTraceSampleDraft::validateBasic 是否校验 replay_locked 一致性。
P9 定向测试、阶段回归、全量测试是否通过。
边界扫描是否无命中。
```

## 2. 上次阻塞项复查

### 2.1 P9 集成测试直接使用 GameState

结果：已修复。

复查命令：

```bash
rg -n "GameState|ObjectDefinition|edible_profile|hunger_delta|health_delta|effect_kind" backend/tests/integration/p9
```

结果：无命中。

说明：P9 no-decision / skipped 集成测试已改为基于 AgentTickRecord 构造链路，不再在 `integration/p9` 中直接创建 `GameState`，符合 P9 “只消费 P8 记录层和 P9 契约”的边界。

### 2.2 trace.phase_keys / trace.reason_keys 语义错误

结果：已修复。

修复位置：

```text
backend/src/agent/training_sample.cpp
```

当前行为：

```text
trace.phase_keys 合并 tick record 的 phase_keys 与 decision_record.phase_keys。
trace.reason_keys 来自 decision_record.reason_key 和 decision_record.rejected_reason_keys。
decision_record.phase_keys 不再写入 trace.reason_keys。
```

新增/确认测试：

```text
agent_training_sample_trace_phase_not_reason
agent_training_sample_trace_rejected_reasons
```

结论：trace 字段语义已恢复正确。

## 3. 上次建议项复查

### 3.1 AgentReplayLockSet::validateBasic 校验 source_hash

结果：已修复。

新增/确认测试：

```text
agent_replay_lock_set_empty_source_hash
```

结论：手工构造的 lock set 缺 source_hash 会失败。

### 3.2 AgentTraceSampleDraft::validateBasic 校验 replay_locked 一致性

结果：已修复。

当前行为：

```text
trace.replay_locked=true 时，replay_lock_status 必须为 Locked。
```

新增/确认测试：

```text
agent_training_sample_trace_replay_locked
```

结论：trace replay lock 状态一致性已受校验保护。

## 4. 测试结果

构建结果：通过。

```bash
cmake -S backend -B build/backend
cmake --build build/backend
```

P9 定向测试：通过。

```text
100% tests passed, 0 tests failed out of 55
```

P3-P9 / Agent / Replay 回归测试：通过。

```text
100% tests passed, 0 tests failed out of 165
```

全量测试：通过。

```text
100% tests passed, 0 tests failed out of 229
```

## 5. 边界扫描结果

运行时 / 管线 / 复盘调用扫描：通过。

```text
扫描目标：replay_lock.h / replay_lock.cpp / training_sample.h / training_sample.cpp
扫描内容：AgentRuntime( / FirstSupportedPolicy / decide( / tickOne( / ReplayRunner / RulePipeline / execute(
结果：无命中
```

Reward / Done / RL 扫描：通过。

```text
扫描内容：reward_value / done = / is_done / RewardCalculator / EpisodeRunner / rl_environment / RLPolicy / NeuralPolicy / LLMPolicy
结果：无命中
```

隐藏真相扫描：通过。

```text
扫描内容：GameState / ObjectDefinition / edible_profile / hunger_delta / health_delta / effect_kind
结果：无命中
```

禁止系统扫描：通过。

```text
扫描内容：SaveManager / WebSocket / HTTP / BehaviorTree / GOAP / FleeResolver / GroupCombat / WarResolver / tribe_split / pack_combat
结果：无命中
```

目录越界扫描：通过。

```text
未发现 frontend / http / websocket / save 相关新增目录
```

## 6. 已确认能力

P9 当前具备：

```text
AgentReplayLockStatus / AgentReplayLockReason 枚举和 roundtrip。
AgentReplayLockEntry / AgentReplayLockSet 基础校验。
AgentReplayLockSetBuilder 可生成 Locked / Broken / ExplainedOnly。
AgentReplayLockSet 可判断 allReplayableWithoutPolicy / hasBrokenEntry。
AgentTrainingSampleDraft 基础 DTO 和校验。
AgentTrainingSampleDraftBuilder 可生成 Draft / ReplayLocked / Skipped / NoDecision 样本。
Reward / Done 保持 NotComputed，不出现 reward_value / done bool。
NoDecision / Skipped 样本必须具备可解释 trace。
trace phase_keys / reason_keys 语义正确。
P8 replay_record_id 透传测试已补齐。
P9 unknown fruit replay locked + training draft 集成测试通过。
P9 no-decision / skipped training draft 集成测试通过。
```

## 7. 非阻塞建议

```text
1. 提交前确认 P9 新增文件全部纳入 git 跟踪。
2. P10 实现时继续保持只读查询和协议投影，不要引入 H5 / HTTP / WebSocket / JSON 序列化。
3. P10 投影 Public 模式必须隐藏 debug trace，Debug/Training 模式也不能暴露 hidden truth。
```

## 8. 最终结论

P9 复审通过。

允许进入下一阶段：

```text
P10：Agent历史查询与协议投影前置。
```
