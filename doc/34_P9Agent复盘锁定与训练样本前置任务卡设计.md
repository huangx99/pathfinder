# P9 Agent复盘锁定与训练样本前置任务卡设计

## 1. P9 要解决什么问题

P8 已经做到：

```text
AgentRuntime 运行一次 tick。
AgentRecordBuilder 把 tick request/result 固化成 AgentTickRecord。
AgentReplayBridge 把成功进管线的记录转成 CommandReplayRecord。
ReplayRunner 可以用 CommandReplayLog 回放，不重新决策。
```

P9 要解决两个更高一层的问题：

```text
1. 如何明确标记一段 Agent 历史已经 replay locked，回放时绝对不能重新调用 Policy。
2. 如何从 AgentTickRecord 派生最小训练样本草稿，让未来强化学习/模仿学习可以接入，但 P9 不实现训练算法。
```

P9 的一句话目标：

```text
让 Agent 历史既能被锁定复盘，又能被安全地转成训练数据草稿。
```

## 2. P9 与原设计的关系

P9 继续依据原有 01-24 设计，不是新架构。

主要依据：

```text
doc/19_存档与复盘设计.md：复盘必须确定性，不能重新抽随机，不能重新决策。
doc/21_Agent系统设计.md：ReplayLocked Agent 不能重新决策；训练步进需要 Observation / Action / Reward / Done / Trace。
doc/22_测试策略设计.md：决策确定性、复盘和训练接口都要可测试。
doc/23_工程实现路线图设计.md：小步任务卡，AI 友好实现。
doc/33_P8Agent运行记录与复盘兼容任务卡设计.md：AgentTickRecord / AgentDecisionRecord / AgentReplayBridge。
```

P9 不改变权威链：

```text
CommandEnvelope 仍是唯一行为输入。
RulePipeline 仍是唯一规则结算入口。
CommandReplayLog 仍是复盘命令来源。
AgentTickRecord 只解释历史。
TrainingSampleDraft 只描述数据，不参与结算。
```

## 3. P9 工程定位

P9 是 P8 之后的两个轻量契约层：

```text
ReplayLock 层：判断一组 AgentTickRecord + CommandReplayLog 是否已经锁定，可安全回放且不重新决策。
TrainingDraft 层：从 AgentTickRecord 派生最小训练样本草稿，未来再扩展 Reward / Done / batch dataset。
```

P9 不做：

```text
不做 RL 算法。
不做 Reward 计算器。
不做 Done 判定器。
不做 Episode 运行器。
不做 batch dataset 文件。
不做 SaveManager。
不做数据库。
不做 HTTP / WebSocket / H5。
不做 LLMPolicy / NeuralPolicy。
不做多 Agent 调度。
不做新玩法 resolver。
```

## 4. P9 最小闭环定义

### 4.1 replay locked 闭环

输入：

```text
AgentTickLog：包含 P8 unknown fruit 的 AgentTickRecord。
CommandReplayLog：包含对应 CommandReplayRecord。
```

流程：

```text
AgentReplayLockSetBuilder 扫描 AgentTickLog。
对每条有 command_id 的记录，在 CommandReplayLog 中找匹配命令记录。
生成 AgentReplayLockSet。
AgentReplayLockSet.validateBasic 通过。
ReplayLock 状态为 Locked。
```

验收：

```text
有命令且 command log 匹配 -> Locked。
有命令但 command log 缺失 -> Broken。
SubmitSkipped / NoDecision -> ExplainedOnly，不要求 CommandReplayRecord。
ReplayLock 过程不调用 AgentRuntime / Policy / ReplayRunner / RulePipeline。
```

### 4.2 training sample draft 闭环

输入：

```text
P8 unknown fruit AgentTickRecord。
```

流程：

```text
AgentTrainingSampleDraftBuilder 从 AgentTickRecord 派生 TrainingSampleDraft。
提取 observation/action/result/trace 最小字段。
标记 reward_status=NotComputed。
标记 done_status=NotComputed。
validateBasic 通过。
```

验收：

```text
样本可关联 agent_id、tick、state_version_before/after、decision_id、command_id。
样本只使用 AgentTickRecord 已有字段。
样本不读取 GameState / ObjectDefinition / hidden truth。
样本不生成 reward 数值。
样本不判定 done。
```

### 4.3 skipped/no-decision sample 闭环

输入：

```text
SubmitSkipped AgentTickRecord。
NoDecision AgentTickRecord。
```

验收：

```text
TrainingSampleDraft 可以生成。
Action 部分标记为 Skipped 或 NoDecision。
command_id 为空时不报错。
trace 必须包含 reason_key 或 phase_keys。
reward/done 仍是 NotComputed。
```

## 5. P9 新增核心类型

### 5.1 AgentReplayLockStatus

文件建议：`backend/include/pathfinder/agent/replay_lock.h`

```cpp
struct AgentReplayLockSetIdTag {};
using AgentReplayLockSetId = foundation::StrongId<AgentReplayLockSetIdTag>;

enum class AgentReplayLockStatus {
    Unknown,
    Locked,
    ExplainedOnly,
    Broken,
    Invalid
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值，不能作为有效状态 |
| Locked | 已锁定 | 有命令记录且可不用 Policy 回放 |
| ExplainedOnly | 仅可解释 | NoDecision / SubmitSkipped，无命令可回放但历史可解释 |
| Broken | 断链 | Agent 记录有命令，但 CommandReplayLog 缺失或不匹配 |
| Invalid | 无效 | 记录本身非法或字段不完整 |

### 5.2 AgentReplayLockReason

```cpp
enum class AgentReplayLockReason {
    Unknown,
    CommandRecordMatched,
    NoCommandExpected,
    CommandRecordMissing,
    CommandIdMismatch,
    AgentRecordInvalid,
    CommandRecordInvalid
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值 |
| CommandRecordMatched | 命令记录匹配 | command_id 找到并匹配 |
| NoCommandExpected | 不期望命令 | SubmitSkipped / NoDecision 正常无命令 |
| CommandRecordMissing | 命令记录缺失 | Agent 记录有 command_id，但日志找不到 |
| CommandIdMismatch | 命令 ID 不一致 | Agent 记录与命令记录不一致 |
| AgentRecordInvalid | Agent 记录非法 | AgentTickRecord validateBasic 失败 |
| CommandRecordInvalid | 命令记录非法 | CommandReplayRecord validateBasic 失败 |

### 5.3 AgentReplayLockEntry

```cpp
struct AgentReplayLockEntry {
    AgentTickRecordId agent_record_id;
    AgentId agent_id;
    foundation::Tick tick;
    foundation::StateVersion state_version_before;
    foundation::StateVersion state_version_after;
    std::optional<foundation::DecisionId> decision_id;
    std::optional<foundation::CommandId> command_id;
    std::optional<replay::ReplayRecordId> replay_record_id;

    AgentReplayLockStatus status = AgentReplayLockStatus::Unknown;
    std::vector<AgentReplayLockReason> reasons;
    std::vector<std::string> reason_keys;

    foundation::Result<void> validateBasic() const;
};
```

约束：

```text
agent_record_id 必须合法。
agent_id 必须合法。
status 不能 Unknown。
Locked 必须有 command_id 和 replay_record_id。
ExplainedOnly 不能强制要求 command_id。
Broken 必须有至少一个 reason。
reason_keys 只存解释 key，不存隐藏真相。
```

### 5.4 AgentReplayLockSet

```cpp
struct AgentReplayLockSet {
    AgentReplayLockSetId lock_set_id;
    std::vector<AgentReplayLockEntry> entries;
    foundation::HashValue source_hash;
    foundation::Result<void> validateBasic() const;

    bool allReplayableWithoutPolicy() const;
    bool hasBrokenEntry() const;
};
```

ID 规则：

```text
lock_set_id = agent_replay_lock_set_<first_tick>_<last_tick>_<entry_count>
```

注意：

```text
必须使用 AgentReplayLockSetId。
不得使用不存在的 foundation::Id。
不得使用中文、空格、冒号、斜杠。
必须提供 helper 生成，不允许研发手拼随机 ID。
```

### 5.5 AgentReplayLockSetBuilder

```cpp
struct AgentReplayLockBuildRequest {
    const AgentTickLog* agent_log = nullptr;
    const replay::CommandReplayLog* command_log = nullptr;
    foundation::HashValue source_hash;

    foundation::Result<void> validateBasic() const;
};

class AgentReplayLockSetBuilder {
public:
    foundation::Result<AgentReplayLockSet> build(
        const AgentReplayLockBuildRequest& request) const;
};
```

职责：

```text
扫描 AgentTickLog。
对每条 AgentTickRecord 建立 AgentReplayLockEntry。
有 command_id 时必须查 CommandReplayLog。
SubmitSkipped / NoDecision 转 ExplainedOnly。
非法记录转 Invalid 或返回 error，具体由实现保持一致。
不调用 AgentRuntime。
不调用 Policy。
不调用 ReplayRunner。
不调用 RulePipeline。
不修改 GameState。
```

## 6. P9 训练样本草稿类型

### 6.1 AgentTrainingSampleStatus

文件建议：`backend/include/pathfinder/agent/training_sample.h`

```cpp
struct AgentTrainingSampleIdTag {};
using AgentTrainingSampleId = foundation::StrongId<AgentTrainingSampleIdTag>;

enum class AgentTrainingSampleStatus {
    Unknown,
    Draft,
    ReplayLocked,
    Skipped,
    NoDecision,
    Invalid
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值，非法有效状态 |
| Draft | 草稿 | 可作为训练样本草稿 |
| ReplayLocked | 复盘锁定 | 样本来自已锁定记录 |
| Skipped | 已跳过 | SubmitSkipped 样本 |
| NoDecision | 无决策 | NoDecision 样本 |
| Invalid | 无效 | 样本字段不完整 |

### 6.2 AgentRewardDraftStatus

```cpp
enum class AgentRewardDraftStatus {
    Unknown,
    NotComputed,
    PlaceholderOnly
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值 |
| NotComputed | 未计算 | P9 默认状态 |
| PlaceholderOnly | 仅占位 | 只为未来字段预留 |

P9 强制要求：

```text
不得出现具体 reward_value。
不得根据 health/hunger/event 猜 reward。
不得定义 RewardCalculator。
```

### 6.3 AgentDoneDraftStatus

```cpp
enum class AgentDoneDraftStatus {
    Unknown,
    NotComputed,
    PlaceholderOnly
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值 |
| NotComputed | 未计算 | P9 默认状态 |
| PlaceholderOnly | 仅占位 | 为未来 episode 结束预留 |

P9 强制要求：

```text
不得判断 episode 是否结束。
不得定义 EpisodeRunner。
不得读取 GameState 判断死亡、胜利、文明阶段。
```

### 6.4 AgentObservationSampleDraft

```cpp
struct AgentObservationSampleDraft {
    AgentId agent_id;
    foundation::EntityId actor_id;
    foundation::Tick tick;
    foundation::StateVersion state_version;
    size_t observed_object_count = 0;
    size_t observed_threat_count = 0;
    size_t knowledge_claim_count = 0;
    foundation::HashValue observation_hash;

    foundation::Result<void> validateBasic() const;
};
```

说明：

```text
P9 不复制完整 Observation。
只记录训练所需最小摘要和 hash。
后续如果需要完整 ObservationDataset，由 P10+ 设计。
```

### 6.5 AgentActionSampleDraft

```cpp
struct AgentActionSampleDraft {
    std::optional<foundation::DecisionId> decision_id;
    std::optional<foundation::ActionId> selected_candidate_id;
    std::optional<foundation::ActionId> selected_command_action_id;
    AgentIntentType intent_type = AgentIntentType::Unknown;
    std::vector<command::ActionTarget> selected_targets;
    std::optional<foundation::CommandId> command_id;
    AgentDecisionRecordStatus decision_status = AgentDecisionRecordStatus::Unknown;

    foundation::Result<void> validateBasic() const;
};
```

约束：

```text
PipelineSucceeded / PipelineFailed 必须有 command_id。
SubmitSkipped 可以有 selected action，但不要求 command_id。
NoDecision 不要求 action_id，但必须能通过 trace 解释。
selected_targets 必须 validateBasic。
```

### 6.6 AgentResultSampleDraft

```cpp
struct AgentResultSampleDraft {
    AgentRuntimeStatus runtime_status = AgentRuntimeStatus::Unknown;
    std::optional<pipeline::PipelineStatus> pipeline_status;
    foundation::StateVersion state_version_before;
    foundation::StateVersion state_version_after;
    size_t state_change_count = 0;
    size_t event_count = 0;
    size_t error_count = 0;
    foundation::HashValue input_hash;
    foundation::HashValue output_hash;

    AgentRewardDraftStatus reward_status = AgentRewardDraftStatus::NotComputed;
    AgentDoneDraftStatus done_status = AgentDoneDraftStatus::NotComputed;

    foundation::Result<void> validateBasic() const;
};
```

约束：

```text
runtime_status 不能 Unknown。
input_hash/output_hash 必须非空。
reward_status 必须 NotComputed 或 PlaceholderOnly。
done_status 必须 NotComputed 或 PlaceholderOnly。
P9 不允许 reward_value 字段。
P9 不允许 done bool 字段。
```

### 6.7 AgentTraceSampleDraft

```cpp
struct AgentTraceSampleDraft {
    std::vector<std::string> phase_keys;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;
    bool replay_locked = false;
    AgentReplayLockStatus replay_lock_status = AgentReplayLockStatus::Unknown;

    foundation::Result<void> validateBasic() const;
};
```

约束：

```text
NoDecision / SubmitSkipped 必须至少有 phase_keys 或 reason_keys。
普通前端未来不直接展示完整 trace，P9 只做数据契约。
```

### 6.8 AgentTrainingSampleDraft

```cpp
struct AgentTrainingSampleDraft {
    AgentTrainingSampleId sample_id;
    AgentTickRecordId source_record_id;
    AgentTrainingSampleStatus status = AgentTrainingSampleStatus::Unknown;

    AgentObservationSampleDraft observation;
    AgentActionSampleDraft action;
    AgentResultSampleDraft result;
    AgentTraceSampleDraft trace;

    foundation::Result<void> validateBasic() const;
};
```

ID 规则：

```text
sample_id = agent_training_sample_<agent_id>_<tick>_<state_version_before>
```

注意：

```text
P9 样本是 Draft，不是最终训练数据集。
不得包含隐藏对象真实效果。
不得包含 ObjectDefinition。
不得包含未遮罩 GameState。
```

### 6.9 AgentTrainingSampleDraftBuilder

```cpp
struct AgentTrainingSampleBuildRequest {
    AgentTickRecord record;
    std::optional<AgentReplayLockEntry> replay_lock_entry;
    foundation::HashValue observation_hash;

    foundation::Result<void> validateBasic() const;
};

class AgentTrainingSampleDraftBuilder {
public:
    foundation::Result<AgentTrainingSampleDraft> build(
        const AgentTrainingSampleBuildRequest& request) const;
};
```

职责：

```text
从 AgentTickRecord 派生 TrainingSampleDraft。
如果提供 ReplayLockEntry，则写入 trace.replay_locked / replay_lock_status。
不读取 GameState。
不读取 ObjectDefinition。
不读取 hidden truth。
不调用 AgentRuntime / Policy / RulePipeline / ReplayRunner。
不计算 Reward。
不判断 Done。
```

## 7. P9 文件范围

### 7.1 允许新增

```text
backend/include/pathfinder/agent/replay_lock.h
backend/include/pathfinder/agent/training_sample.h
backend/src/agent/replay_lock.cpp
backend/src/agent/training_sample.cpp
backend/tests/unit/agent/agent_replay_lock_test.cpp
backend/tests/unit/agent/agent_training_sample_test.cpp
backend/tests/integration/p9/agent_replay_locked_training_draft_test.cpp
backend/tests/integration/p9/agent_no_decision_training_draft_test.cpp
context_packs/agent_p9.md
```

### 7.2 允许修改

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
```

可选补强：

```text
backend/tests/unit/agent/agent_replay_bridge_test.cpp
```

仅允许补 `replay_record_id` 透传测试，不允许改变 P8 语义。

### 7.3 禁止修改

```text
backend/src/rules/
backend/include/pathfinder/rules/
backend/src/object/
backend/include/pathfinder/object/
backend/src/pipeline/
backend/include/pathfinder/pipeline/
backend/src/replay/replay_runner.cpp
frontend/
任何 HTTP / WebSocket / SaveManager 目录。
```

## 8. 必读文档

P9 实现 AI 必须读取：

```text
doc/34_P9Agent复盘锁定与训练样本前置任务卡设计.md
context_packs/agent_p9.md
context_packs/agent_p8.md
context_packs/replay_p4.md
doc/21_Agent系统设计.md 的 ReplayLocked / 强化学习关系 / 边界规则片段
doc/33_P8Agent运行记录与复盘兼容任务卡设计.md
backend/dev_notes/agent.md
```

最多再读取：

```text
backend/include/pathfinder/agent/record_types.h
backend/include/pathfinder/agent/replay_bridge.h
backend/include/pathfinder/replay/command_replay.h
backend/include/pathfinder/replay/replay_runner.h
backend/include/pathfinder/agent/observation.h
backend/include/pathfinder/agent/action_space.h
```

禁止为了 P9 一次性读取：

```text
01-20 全部文档。
传播、族群、文明完整设计。
H5 / 协议完整设计。
完整 SaveManager 设计。
```

## 9. TASK-P9-000：上下文包

任务名称：创建 P9 上下文包。

允许新增：

```text
context_packs/agent_p9.md
```

必须包含：

```text
P9 目标。
P9 与 P8/P4 的关系。
ReplayLocked 原则。
TrainingSampleDraft 原则。
明确 P9 不做 Reward / Done / RL 算法。
文件范围。
禁止修改范围。
验收命令。
```

验收命令：

```bash
rg -n "P9|ReplayLock|TrainingSampleDraft|Reward|Done|不实现 RL" context_packs/agent_p9.md
```

## 10. TASK-P9-001：实现 ReplayLock 基础类型

任务名称：实现 AgentReplayLockStatus / AgentReplayLockEntry / AgentReplayLockSet。

允许新增：

```text
backend/include/pathfinder/agent/replay_lock.h
backend/src/agent/replay_lock.cpp
backend/tests/unit/agent/agent_replay_lock_test.cpp
```

允许修改：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

实现要求：

```text
定义 AgentReplayLockStatus。
定义 AgentReplayLockReason。
实现 toString/fromString。
实现 AgentReplayLockEntry::validateBasic。
实现 AgentReplayLockSet::validateBasic。
实现 allReplayableWithoutPolicy。
实现 hasBrokenEntry。
实现稳定 lock_set_id helper。
不依赖 RulePipeline。
不依赖 GameState。
不读取 hidden truth。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_replay_lock --output-on-failure
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|RulePipeline|execute\(|GameState|edible_profile|hunger_delta|health_delta|effect_kind" backend/include/pathfinder/agent/replay_lock.h backend/src/agent/replay_lock.cpp backend/tests/unit/agent/agent_replay_lock_test.cpp && exit 1 || true
```

测试要求：

```text
枚举 roundtrip 覆盖全部枚举。
Locked entry 缺 command_id 失败。
Locked entry 缺 replay_record_id 失败。
ExplainedOnly 不要求 command_id。
Broken entry 缺 reason 失败。
valid lock set 通过。
empty lock set 失败。
allReplayableWithoutPolicy 对全 Locked 返回 true。
hasBrokenEntry 对 Broken 返回 true。
```

## 11. TASK-P9-002：实现 ReplayLockSetBuilder

任务名称：从 AgentTickLog + CommandReplayLog 构建 replay lock set。

允许修改：

```text
backend/include/pathfinder/agent/replay_lock.h
backend/src/agent/replay_lock.cpp
backend/tests/unit/agent/agent_replay_lock_test.cpp
```

实现要求：

```text
定义 AgentReplayLockBuildRequest。
实现 AgentReplayLockSetBuilder::build。
PipelineSucceeded / PipelineFailed 且 command_id 匹配 -> Locked。
SubmitSkipped / NoDecision -> ExplainedOnly。
有 command_id 但 CommandReplayLog 找不到 -> Broken。
非法 AgentTickRecord -> Invalid 或 error，保持测试明确。
不调用 AgentRuntime。
不调用 Policy。
不调用 ReplayRunner。
不调用 RulePipeline。
不修改 GameState。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_replay_lock --output-on-failure
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|ReplayRunner|RulePipeline|execute\(" backend/include/pathfinder/agent/replay_lock.h backend/src/agent/replay_lock.cpp backend/tests/unit/agent/agent_replay_lock_test.cpp && exit 1 || true
```

测试要求：

```text
matching command log -> Locked。
missing command log -> Broken。
SubmitSkipped -> ExplainedOnly。
NoDecision -> ExplainedOnly。
source_hash 为空失败。
duplicate agent records 由 AgentTickLog 保证，Builder 不重复实现。
```

## 12. TASK-P9-003：实现 TrainingSampleDraft 基础类型

任务名称：实现训练样本草稿数据契约。

允许新增：

```text
backend/include/pathfinder/agent/training_sample.h
backend/src/agent/training_sample.cpp
backend/tests/unit/agent/agent_training_sample_test.cpp
```

允许修改：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

实现要求：

```text
定义 AgentTrainingSampleStatus。
定义 AgentRewardDraftStatus。
定义 AgentDoneDraftStatus。
定义 AgentObservationSampleDraft。
定义 AgentActionSampleDraft。
定义 AgentResultSampleDraft。
定义 AgentTraceSampleDraft。
定义 AgentTrainingSampleDraft。
实现 toString/fromString。
实现全部 validateBasic。
实现稳定 sample_id helper。
不出现 reward_value 字段。
不出现 done bool 字段。
不依赖 GameState / ObjectDefinition。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_training_sample --output-on-failure
rg -n "reward_value|done =|is_done|EpisodeRunner|RewardCalculator|GameState|ObjectDefinition|edible_profile|hunger_delta|health_delta|effect_kind" backend/include/pathfinder/agent/training_sample.h backend/src/agent/training_sample.cpp backend/tests/unit/agent/agent_training_sample_test.cpp && exit 1 || true
```

测试要求：

```text
枚举 roundtrip 覆盖全部枚举。
合法 sample 通过。
缺 sample_id 失败。
缺 source_record_id 失败。
Unknown status 失败。
result runtime_status Unknown 失败。
input_hash/output_hash 为空失败。
reward_status Unknown 失败。
done_status Unknown 失败。
PipelineSucceeded action 缺 command_id 失败。
NoDecision action 不要求 command_id。
NoDecision trace 无 reason/phase 失败。
```

## 13. TASK-P9-004：实现 TrainingSampleDraftBuilder

任务名称：从 AgentTickRecord 派生训练样本草稿。

允许修改：

```text
backend/include/pathfinder/agent/training_sample.h
backend/src/agent/training_sample.cpp
backend/tests/unit/agent/agent_training_sample_test.cpp
```

实现要求：

```text
定义 AgentTrainingSampleBuildRequest。
实现 AgentTrainingSampleDraftBuilder::build。
从 AgentTickRecord 填充 observation/action/result/trace。
observation_hash 由 request 传入，P9 不实现完整 Observation hash 算法。
ReplayLockEntry 可选，存在时写入 trace replay lock 信息。
PipelineSucceeded/PipelineFailed -> status Draft 或 ReplayLocked。
SubmitSkipped -> status Skipped。
NoDecision -> status NoDecision。
reward_status 固定 NotComputed。
done_status 固定 NotComputed。
不调用 AgentRuntime / Policy / RulePipeline / ReplayRunner。
不读取 GameState / hidden truth。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_training_sample --output-on-failure
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|RulePipeline|execute\(|ReplayRunner|GameState|ObjectDefinition|reward_value|is_done" backend/include/pathfinder/agent/training_sample.h backend/src/agent/training_sample.cpp backend/tests/unit/agent/agent_training_sample_test.cpp && exit 1 || true
```

测试要求：

```text
PipelineSucceeded record -> sample Draft。
Locked replay entry -> sample ReplayLocked 且 trace.replay_locked=true。
SubmitSkipped record -> sample Skipped。
NoDecision record -> sample NoDecision。
缺 observation_hash 返回 error。
sample_id deterministic。
state_change_count / event_count / error_count 保持一致。
```

## 14. TASK-P9-005：P9 unknown fruit 集成测试

任务名称：证明 P8 unknown fruit 记录可以进入 replay locked，并派生训练草稿。

允许新增：

```text
backend/tests/integration/p9/agent_replay_locked_training_draft_test.cpp
```

允许修改：

```text
backend/tests/CMakeLists.txt
```

实现流程：

```text
1. 复用 P8 unknown fruit 场景生成 AgentTickRecord。
2. 使用 AgentReplayBridge 生成 CommandReplayLog。
3. 使用 AgentReplayLockSetBuilder 生成 AgentReplayLockSet。
4. 断言 lock set allReplayableWithoutPolicy=true。
5. 使用 AgentTrainingSampleDraftBuilder 生成 sample。
6. 断言 sample.status=ReplayLocked。
7. 断言 reward_status=NotComputed。
8. 断言 done_status=NotComputed。
9. 可选：ReplayRunner 回放仍 Match，但 TrainingSampleBuilder 不调用 ReplayRunner。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R p9_agent_replay_locked_training_draft --output-on-failure
```

强制要求：

```text
训练样本派生阶段不得创建 AgentRuntime。
训练样本派生阶段不得调用 Policy。
训练样本派生阶段不得调用 ReplayRunner。
训练样本不得包含 reward_value / done bool。
```

## 15. TASK-P9-006：P9 no-decision / skipped 集成测试

任务名称：证明无命令记录也能形成可解释样本，但不能冒充 replay locked。

允许新增：

```text
backend/tests/integration/p9/agent_no_decision_training_draft_test.cpp
```

允许修改：

```text
backend/tests/CMakeLists.txt
```

实现流程：

```text
1. 复用 P8 SubmitSkipped 场景生成 AgentTickRecord。
2. ReplayLockSetBuilder 生成 ExplainedOnly entry。
3. TrainingSampleDraftBuilder 生成 Skipped sample。
4. 复用 P8 NoDecision 场景生成 AgentTickRecord。
5. ReplayLockSetBuilder 生成 ExplainedOnly entry。
6. TrainingSampleDraftBuilder 生成 NoDecision sample。
7. 断言两类 sample 都 reward/done NotComputed。
8. 断言两类 sample 都没有 command replay 伪造。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R p9_agent_no_decision_training_draft --output-on-failure
```

测试要求：

```text
SubmitSkipped -> replay lock status ExplainedOnly。
NoDecision -> replay lock status ExplainedOnly。
Skipped sample 不要求 command_id。
NoDecision sample 不要求 command_id。
NoDecision trace 必须有 reason_keys 或 phase_keys。
```

## 16. TASK-P9-007：补 P8 replay_record_id 透传测试

任务名称：补齐 P8 非阻塞建议的回归测试。

允许修改：

```text
backend/tests/unit/agent/agent_replay_bridge_test.cpp
backend/tests/CMakeLists.txt
```

实现要求：

```text
构造 AgentTickRecord。
设置 decision_record.replay_record_id = ReplayRecordId("agent_replay_custom_001")。
调用 AgentReplayBridge::toCommandReplayRecord。
断言 CommandReplayRecord.record_id 使用自定义 ID。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_replay_bridge --output-on-failure
```

## 17. TASK-P9-008：边界审计和开发笔记

任务名称：更新 Agent 开发笔记并执行 P9 审计。

允许修改：

```text
backend/dev_notes/agent.md
doc/review/P9Agent复盘锁定与训练样本前置审核报告.md
```

审计要求：

```text
确认 ReplayLockSetBuilder 不调用 AgentRuntime / Policy / ReplayRunner / RulePipeline。
确认 TrainingSampleDraftBuilder 不调用 AgentRuntime / Policy / ReplayRunner / RulePipeline。
确认 TrainingSampleDraft 不包含 reward_value / done bool。
确认没有 RewardCalculator / EpisodeRunner。
确认没有读取 GameState / ObjectDefinition / hidden truth。
确认没有 SaveManager / H5 / HTTP / WebSocket / RL 算法。
确认 P8 / P7 / P4 回归测试通过。
确认 P9 集成测试通过。
```

验收命令：

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent_replay_lock|agent_training_sample|p9" --output-on-failure
ctest --test-dir build/backend -R "agent|p9|p8|p7|p6|p5|p4|p3|replay" --output-on-failure
ctest --test-dir build/backend --output-on-failure
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|ReplayRunner|RulePipeline|execute\(" backend/include/pathfinder/agent/replay_lock.h backend/src/agent/replay_lock.cpp backend/include/pathfinder/agent/training_sample.h backend/src/agent/training_sample.cpp && exit 1 || true
rg -n "reward_value|done =|is_done|RewardCalculator|EpisodeRunner|rl_environment|RLPolicy|NeuralPolicy|LLMPolicy" backend/include/pathfinder/agent backend/src/agent backend/tests/unit/agent backend/tests/integration/p9 && exit 1 || true
rg -n "GameState|ObjectDefinition|edible_profile|hunger_delta|health_delta|effect_kind" backend/include/pathfinder/agent/replay_lock.h backend/src/agent/replay_lock.cpp backend/include/pathfinder/agent/training_sample.h backend/src/agent/training_sample.cpp backend/tests/unit/agent/agent_replay_lock_test.cpp backend/tests/unit/agent/agent_training_sample_test.cpp backend/tests/integration/p9 && exit 1 || true
rg -n "SaveManager|WebSocket|HTTP|BehaviorTree|GOAP|FleeResolver|GroupCombat|WarResolver|tribe_split|pack_combat" backend/include/pathfinder/agent backend/src/agent backend/tests/integration/p9 && exit 1 || true
find backend -type d \( -path '*/frontend*' -o -path '*/http*' -o -path '*/websocket*' -o -path '*/save*' \) -print
```

审核报告必须包含：

```text
构建结果。
P9 定向测试结果。
P4-P8 回归测试结果。
全量测试结果。
边界扫描结果。
是否允许进入 P10。
```

## 18. P9 设计风险和处理

### 18.1 风险：把训练样本当成训练系统

处理：

```text
P9 只做 Draft。
reward_status 固定 NotComputed。
done_status 固定 NotComputed。
禁止 reward_value / done bool。
禁止 RewardCalculator / EpisodeRunner。
```

### 18.2 风险：复盘锁定仍然重新决策

处理：

```text
ReplayLock 只检查 P8 记录和 CommandReplayLog。
不调用 AgentRuntime。
不调用 Policy。
边界扫描禁止 decide / tickOne。
```

### 18.3 风险：样本泄露隐藏真相

处理：

```text
TrainingSampleDraftBuilder 只读 AgentTickRecord。
不读 GameState。
不读 ObjectDefinition。
边界扫描禁止 edible_profile / hunger_delta / health_delta / effect_kind。
```

### 18.4 风险：NoDecision 样本不可解释

处理：

```text
P8 已强制 NoDecision 必须 reason_key 或 phase_keys。
P9 TraceSampleDraft 对 NoDecision / SubmitSkipped 继续强制 reason_keys 或 phase_keys。
```

### 18.5 风险：P9 范围膨胀

处理：

```text
不做 H5。
不做协议投影。
不做 SaveManager。
不做数据集文件。
不做多 Agent episode。
这些放到 P10+。
```

## 19. P9 完成后的系统能力

P9 完成后，系统具备：

```text
Agent 历史可以形成 replay lock set。
系统能判断哪些 Agent 历史可不用 Policy 回放。
系统能区分 Locked / ExplainedOnly / Broken。
AgentTickRecord 可以派生最小 TrainingSampleDraft。
训练样本草稿包含 observation/action/result/trace 四段结构。
训练样本草稿明确 reward/done 未计算。
NoDecision / SubmitSkipped 也能成为可解释样本。
后续 P10 可以基于这些草稿继续设计协议投影、数据集导出或训练环境。
```

P9 完成后仍不具备：

```text
真正强化学习。
Reward 计算。
Done 判定。
Episode 管理。
训练 batch。
数据集落盘。
H5 展示。
存档管理。
多 Agent 并行训练。
```

## 20. P9 之后建议

P9 之后建议进入 P10：

```text
P10：Agent历史查询与协议投影前置。
```

P10 建议目标：

```text
把 AgentTickRecord / ReplayLockSet / TrainingSampleDraft 转成前端和工具可读的只读 projection。
仍不做 H5 页面，只做协议 DTO 和查询接口边界。
```

如果希望继续推训练方向，也可以 P10 改为：

```text
P10：训练数据集导出草案。
```

但当前更建议先做协议投影，因为 H5 和调试工具最终都需要读 Agent 历史。
