# 33_P8Agent运行记录与复盘兼容任务卡设计

设计日期：2026-05-17  
阶段：P8  
阶段名称：Agent 运行记录与复盘兼容  
结论：P8 只实现 Agent tick / decision 的内存记录、ReplayBridge 和回放锁定验证，不实现 SaveManager、文件落盘、H5、强化学习算法或完整 AgentService。

## 1. P8 要解决什么问题

P7 已经完成最小 AgentRuntime：

```text
ObservationBuilder -> ActionSpaceBuilder -> FirstSupportedPolicy -> AgentCommandAdapter -> RulePipeline
```

但 P7 的结果还只是一次运行结果，缺少长期工程必须具备的记录能力：

```text
这次 Agent 看到了什么？
动作空间里有哪些候选？
它为什么选择这个动作？
选择生成了哪个 CommandEnvelope？
命令是否进入 RulePipeline？
产生了哪些 StateChange / Event？
复盘时是否可以解释当时为什么行动，而不重新调用 Policy？
后续训练环境能否拿到 Observation / Action / Result / Trace 的稳定样本？
```

P8 就是把 P7 的 `AgentTickResult` 固化成内存级 `AgentTickRecord` / `AgentDecisionRecord`，并与 P4 的 `CommandReplayLog` / `ReplayRunner` 对齐。

P8 的核心目标：

```text
历史运行必须可解释。
历史命令必须可复盘。
复盘时不能重新思考。
训练样本可以从记录中派生，但 P8 不做训练算法。
```

## 2. P8 与原设计的关系

P8 继续依据原有设计，不是新架构。

主要依据：

```text
doc/19_存档与复盘设计.md：CommandReplayRecord、RandomReplayRecord、ReplayRunner、回放禁止重新抽随机。
doc/21_Agent系统设计.md：AgentDecisionRecord、AgentDecisionTrace、ReplayLocked、训练样本前置。
doc/23_工程实现路线图设计.md：AI 任务卡、小步实现、复盘和测试优先。
doc/32_P7Agent运行时最小闭环任务卡设计.md：AgentRuntime 最小调度闭环和 submit_action_allowlist。
```

P8 不改变规则权威链：

```text
Agent 只记录决策。
CommandEnvelope 仍由 AgentCommandAdapter 生成。
RulePipeline 仍是唯一规则结算入口。
ReplayRunner 仍通过 RulePipeline 回放命令。
AgentDecisionRecord 只解释历史，不参与重新结算。
```

## 3. P8 工程定位

P8 是 Agent Runtime 的“记录层”和“复盘桥接层”。

三层关系：

```text
P7 Runtime：执行一次 Agent tick，得到 AgentTickResult。
P8 Record：把 request/result 固化成 AgentTickRecord / AgentDecisionRecord。
P8 ReplayBridge：把 AgentTickRecord 中的命令执行部分转成 CommandReplayRecord，交给 P4 ReplayRunner。
```

P8 不做：

```text
不做 SaveManager。
不做文件格式。
不做数据库。
不做 HTTP / WebSocket / H5。
不做完整 AgentService / AgentStore。
不做 Replay UI。
不做 RL Policy / Reward 算法。
不做多 Agent 并发调度。
不做新玩法规则。
```

## 4. P8 最小闭环定义

### 4.1 unknown fruit 记录闭环

输入：

```text
UnknownFruitFixture::createInitialState()
FirstSupportedPolicy
AgentRuntime.tickOne
VisibilityInput: obj_unknown_fruit_001
submit_action_allowlist=[eat]
```

流程：

```text
执行 P7 Runtime。
得到 AgentTickResult(PipelineSucceeded)。
AgentRecordBuilder 从 request/result 生成 AgentTickRecord。
AgentTickLog append record。
AgentReplayBridge 从 AgentTickRecord 生成 CommandReplayRecord。
CommandReplayLog append replay record。
ReplayRunner 使用 CommandReplayLog 回放命令。
ReplayRunner 得到 Match。
```

验收：

```text
AgentTickRecord 合法。
AgentDecisionRecord 合法。
CommandReplayRecord 合法。
ReplayRunner 回放结果 Match。
回放阶段不调用 AgentRuntime / Policy。
```

### 4.2 no-submit 记录闭环

输入：

```text
Fire threat seed。
FirstSupportedPolicy 选择 Flee。
submit_action_allowlist=[eat]。
Runtime status=SubmitSkipped。
```

验收：

```text
AgentTickRecord 合法。
AgentDecisionRecord 可以记录 Flee 决策。
没有 CommandReplayRecord，因为未提交 RulePipeline。
AgentReplayBridge 对 SubmitSkipped 返回明确 error 或 no-command result，不能伪造 replay record。
```

### 4.3 no-decision 记录闭环

输入：

```text
Social threat seed。
CallGroup candidate command_supported=false。
Policy 无 supported candidate。
Runtime status=NoDecision。
```

验收：

```text
AgentTickRecord 合法。
AgentDecisionRecord 可以为空或 status=NoDecision。
没有 CommandEnvelope。
没有 CommandReplayRecord。
记录能解释为什么没有行动。
```

## 5. P8 新增核心类型

### 5.1 AgentRecordId

文件建议：`backend/include/pathfinder/agent/record_types.h`

```cpp
struct AgentTickRecordIdTag {};
struct AgentDecisionRecordIdTag {};

using AgentTickRecordId = foundation::StrongId<AgentTickRecordIdTag>;
using AgentDecisionRecordId = foundation::StrongId<AgentDecisionRecordIdTag>;
```

ID 规则：

```text
AgentTickRecordId: agent_tick_<agent_id>_<tick>_<state_version>
AgentDecisionRecordId: agent_decision_record_<decision_id>
```

注意：

```text
ID 必须符合 isValidIdString。
不得使用中文、空格、冒号、斜杠。
不得让研发手工随便拼不稳定 ID。
必须提供统一 helper 生成。
```

### 5.2 AgentDecisionRecordStatus

```cpp
enum class AgentDecisionRecordStatus {
    Unknown,
    Selected,
    NoDecision,
    AdapterRejected,
    SubmitSkipped,
    PipelineSucceeded,
    PipelineFailed,
    Failed
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值，非法有效状态 |
| Selected | 已选择 | Policy 已选择意图，但未必提交 |
| NoDecision | 无决策 | 没有 supported candidate |
| AdapterRejected | 适配拒绝 | Adapter 拒绝生成命令 |
| SubmitSkipped | 跳过提交 | allowlist 或配置导致不提交 |
| PipelineSucceeded | 管线成功 | 命令进入管线并成功 |
| PipelineFailed | 管线失败 | 命令进入管线但失败 |
| Failed | 失败 | Runtime 或记录构建失败 |

### 5.3 AgentTickRecordStatus

```cpp
enum class AgentTickRecordStatus {
    Unknown,
    Recorded,
    ReplayLinked,
    ReplayMatched,
    ReplayMismatched,
    Invalid
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值，非法有效状态 |
| Recorded | 已记录 | 已从 Runtime 结果记录 |
| ReplayLinked | 已关联复盘 | 已生成 CommandReplayRecord |
| ReplayMatched | 复盘匹配 | 回放成功匹配 |
| ReplayMismatched | 复盘不匹配 | 回放产生差异 |
| Invalid | 无效 | 记录校验失败或不可用 |

### 5.4 AgentDecisionRecord

```cpp
struct AgentDecisionRecord {
    AgentDecisionRecordId record_id;
    foundation::DecisionId decision_id;
    AgentId agent_id;
    foundation::EntityId actor_id;
    foundation::Tick decision_tick;
    foundation::StateVersion observation_state_version;

    foundation::ActionId selected_candidate_id;
    foundation::ActionId selected_command_action_id;
    AgentIntentType selected_intent_type;
    foundation::ActionId selected_intent_action_id;
    std::vector<command::ActionTarget> selected_targets;

    AgentDecisionRecordStatus status = AgentDecisionRecordStatus::Unknown;
    std::string reason_key;
    double confidence = 0.0;
    std::vector<std::string> phase_keys;
    std::vector<std::string> rejected_reason_keys;

    std::optional<foundation::CommandId> command_id;
    std::optional<replay::ReplayRecordId> replay_record_id;

    foundation::Result<void> validateBasic() const;
};
```

约束：

```text
record_id 必须合法。
agent_id 必须合法。
status 不能 Unknown。
如果 status=PipelineSucceeded/PipelineFailed，则 command_id 必须存在。
如果 status=NoDecision，则允许 decision_id 为空，但必须有 reason_key 或 phase_keys 解释。
selected_targets 必须全部 validateBasic。
confidence 必须在 [0.0, 1.0]。
```

### 5.5 AgentTickRecord

```cpp
struct AgentTickRecord {
    AgentTickRecordId record_id;
    AgentId agent_id;
    foundation::EntityId actor_id;
    foundation::Tick tick;
    foundation::StateVersion state_version_before;
    foundation::StateVersion state_version_after;
    foundation::RandomSeed random_seed;

    AgentRuntimeStatus runtime_status;
    AgentTickRecordStatus record_status = AgentTickRecordStatus::Unknown;

    AgentDecisionRecord decision_record;
    std::optional<command::CommandEnvelope> command;
    std::optional<pipeline::PipelineStatus> pipeline_status;
    std::vector<foundation::StateChangeId> state_change_ids;
    std::vector<foundation::EventId> event_ids;
    size_t error_count = 0;

    foundation::HashValue input_hash;
    foundation::HashValue output_hash;

    std::vector<std::string> phase_keys;
    std::vector<std::string> warning_keys;

    foundation::Result<void> validateBasic() const;
};
```

约束：

```text
record_id 必须合法。
record_status 不能 Unknown。
runtime_status 不能 Unknown。
如果 command 存在，command_id 必须和 command.command_id 一致。
如果 pipeline_status 存在，command 必须存在。
如果 runtime_status=PipelineSucceeded，则 pipeline_status 必须 Succeeded。
如果 runtime_status=SubmitSkipped/NoDecision，则不能要求 command replay record。
input_hash/output_hash 必须非空。
```

### 5.6 AgentTickLog

```cpp
class AgentTickLog {
public:
    Result<void> append(AgentTickRecord record);
    size_t size() const;
    const vector<AgentTickRecord>& records() const;
    optional<AgentTickRecord> findByRecordId(AgentTickRecordId id) const;
    vector<AgentTickRecord> findByAgentId(AgentId agent_id) const;
    optional<AgentTickRecord> findByDecisionId(DecisionId decision_id) const;
    Result<void> validateBasic() const;
};
```

约束：

```text
append 必须拒绝重复 record_id。
append 必须先 validateBasic。
find 返回 copy 或 const ref 均可，但不能暴露可变内部状态。
```

### 5.7 AgentRecordBuildRequest

```cpp
struct AgentRecordBuildRequest {
    AgentTickRequest tick_request;
    AgentTickResult tick_result;
    foundation::HashValue input_hash;
    foundation::HashValue output_hash;

    foundation::Result<void> validateBasic() const;
};
```

注意：

```text
P8 不要求实现完整状态哈希算法。
可以复用已有 HashValue 工具或测试中传入稳定 hash。
但 record validate 必须要求 hash 非空，避免后续复盘缺校验点。
```

### 5.8 AgentRecordBuilder

```cpp
class AgentRecordBuilder {
public:
    Result<AgentTickRecord> build(const AgentRecordBuildRequest& request) const;
};
```

职责：

```text
从 AgentTickRequest / AgentTickResult 生成 AgentTickRecord。
提取 selected_candidate_id / selected_command_action_id。
提取 command_id。
提取 PipelineResult 中的 state_change_ids / event_ids / status / errors。
根据 runtime_status 映射 AgentDecisionRecordStatus。
生成稳定 record_id。
不调用 AgentRuntime。
不调用 Policy。
不调用 RulePipeline。
不读取 GameState 隐藏真相。
```

### 5.9 AgentReplayBridge

文件建议：`backend/include/pathfinder/agent/replay_bridge.h`

```cpp
class AgentReplayBridge {
public:
    Result<replay::CommandReplayRecord> toCommandReplayRecord(
        const AgentTickRecord& record) const;

    Result<void> appendCommandReplayRecord(
        const AgentTickRecord& record,
        replay::CommandReplayLog& command_log) const;
};
```

职责：

```text
把可提交且已进管线的 AgentTickRecord 转成 P4 CommandReplayRecord。
不执行 ReplayRunner。
不调用 RulePipeline。
不修改 GameState。
SubmitSkipped / NoDecision / Failed 不能生成 CommandReplayRecord。
```

转换规则：

```text
CommandReplayRecord.record_id = record.decision_record.replay_record_id 或稳定生成 agent_replay_<command_id>。
CommandReplayRecord.command_id = record.command.command_id。
CommandReplayRecord.command = record.command。
CommandReplayRecord.state_version_before = record.state_version_before。
CommandReplayRecord.state_version_after = record.state_version_after。
CommandReplayRecord.random_seed = record.random_seed。
CommandReplayRecord.input_hash = record.input_hash。
CommandReplayRecord.output_hash = record.output_hash。
CommandReplayRecord.state_change_ids = record.state_change_ids。
CommandReplayRecord.event_ids = record.event_ids。
CommandReplayRecord.pipeline_status = record.pipeline_status。
CommandReplayRecord.error_count = record.error_count。
CommandReplayRecord.status = Recorded。
```

### 5.10 AgentReplayLockCheck

P8 不实现完整 ReplayLocked Agent，但要实现最小锁定检查。

```cpp
struct AgentReplayLockCheckResult {
    bool can_replay_without_policy = false;
    bool has_agent_record = false;
    bool has_command_record = false;
    std::vector<std::string> reason_keys;
};

class AgentReplayLockChecker {
public:
    AgentReplayLockCheckResult check(
        const AgentTickRecord& agent_record,
        const replay::CommandReplayLog& command_log) const;
};
```

含义：

```text
如果 AgentTickRecord 有 command_id，CommandReplayLog 必须能找到同 command_id 的记录。
如果找得到，则回放可以不调用 Policy。
如果找不到，则不能宣称 replay locked。
NoDecision / SubmitSkipped 可以没有 command record，但必须有 agent record 解释。
```

## 6. P8 不做什么

P8 禁止实现：

```text
SaveManager。
文件落盘格式。
数据库。
HTTP / WebSocket / H5。
完整 Replay UI。
完整 ReplayService。
完整 AgentService / AgentStore。
多 Agent 调度。
AgentGoal。
Reward 计算器。
强化学习环境。
RL Policy / NeuralPolicy / LLMPolicy。
训练批处理。
新规则 resolver。
FleeResolver / GroupCombat / WarResolver / tribe_split。
```

P8 也禁止：

```text
回放时调用 AgentRuntime 重新决策。
回放时调用 Policy。
AgentReplayBridge 调用 RulePipeline。
AgentRecordBuilder 调用 RulePipeline。
AgentRecordBuilder 修改 GameState。
为了记录方便读取 edible_profile / hidden truth。
为了回放方便绕过 ReplayRunner。
```

## 7. 文件范围

### 7.1 允许新增

```text
backend/include/pathfinder/agent/record_types.h
backend/include/pathfinder/agent/record_builder.h
backend/include/pathfinder/agent/replay_bridge.h
backend/src/agent/record_types.cpp
backend/src/agent/record_builder.cpp
backend/src/agent/replay_bridge.cpp
backend/tests/unit/agent/agent_record_types_test.cpp
backend/tests/unit/agent/agent_record_builder_test.cpp
backend/tests/unit/agent/agent_replay_bridge_test.cpp
backend/tests/integration/p8/agent_runtime_record_unknown_fruit_replay_test.cpp
backend/tests/integration/p8/agent_runtime_record_no_submit_test.cpp
context_packs/agent_p8.md
```

### 7.2 允许修改

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
```

如确有必要，可修改：

```text
backend/include/pathfinder/agent/policy.h
```

仅允许补 `<vector>` include，不允许改变策略语义。

### 7.3 禁止修改

```text
backend/src/rules/
backend/include/pathfinder/rules/
backend/src/object/
backend/include/pathfinder/object/
backend/src/pipeline/rule_pipeline.cpp
backend/src/replay/replay_runner.cpp，除非发现 P4/P7 阻塞 bug。
frontend/
任何 HTTP / WebSocket / SaveManager 目录。
```

## 8. 必读文档

P8 实现 AI 必须读取：

```text
doc/33_P8Agent运行记录与复盘兼容任务卡设计.md
context_packs/agent_p7.md
context_packs/replay_p4.md
doc/21_Agent系统设计.md 的 AgentDecision / AgentDecisionTrace / 存档与复盘 / 强化学习训练片段
doc/19_存档与复盘设计.md 的 CommandReplayRecord / ReplayRunner / 回放禁止项
backend/dev_notes/agent.md
```

最多再读取：

```text
backend/include/pathfinder/agent/runtime_types.h
backend/include/pathfinder/agent/runtime.h
backend/include/pathfinder/replay/command_replay.h
backend/include/pathfinder/replay/replay_runner.h
backend/include/pathfinder/replay/random_replay.h
```

禁止为了 P8 一次性读取：

```text
01-20 全部文档。
传播、族群、文明完整设计。
H5 / 协议完整设计。
完整 SaveManager 设计。
```

## 9. TASK-P8-000：上下文包

任务名称：创建 P8 上下文包。

允许新增：

```text
context_packs/agent_p8.md
```

必须包含：

```text
P8 目标。
允许实现类。
禁止实现类。
文件范围。
P8 与 P4/P7 的关系。
Replay locked 原则。
训练样本前置但不实现 RL 的说明。
验收命令。
```

验收：

```bash
rg -n "P8|AgentTickRecord|AgentDecisionRecord|ReplayBridge|ReplayLocked|CommandReplayRecord" context_packs/agent_p8.md
```

## 10. TASK-P8-001：实现 Agent Record 基础类型

任务名称：实现 AgentTickRecord / AgentDecisionRecord 数据契约。

允许新增：

```text
backend/include/pathfinder/agent/record_types.h
backend/src/agent/record_types.cpp
backend/tests/unit/agent/agent_record_types_test.cpp
```

允许修改：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

实现要求：

```text
定义 AgentTickRecordId / AgentDecisionRecordId。
定义 AgentDecisionRecordStatus / AgentTickRecordStatus。
实现 toString/fromString。
实现 AgentDecisionRecord::validateBasic。
实现 AgentTickRecord::validateBasic。
实现稳定 ID helper。
不依赖 RulePipeline。
不依赖 GameState。
不读取 object/cognition 隐藏真相。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_record_types --output-on-failure
rg -n "GameState|RulePipeline|EatObjectResolver|edible_profile|hunger_delta|health_delta|effect_kind" backend/include/pathfinder/agent/record_types.h backend/src/agent/record_types.cpp backend/tests/unit/agent/agent_record_types_test.cpp && exit 1 || true
```

测试要求：

```text
合法 AgentDecisionRecord 通过。
缺 record_id 失败。
status=Unknown 失败。
confidence 越界失败。
PipelineSucceeded 但缺 command_id 失败。
合法 AgentTickRecord 通过。
缺 input_hash/output_hash 失败。
runtime_status=PipelineSucceeded 但缺 command/pipeline_status 失败。
SubmitSkipped 不要求 command replay record。
枚举 roundtrip 覆盖全部枚举。
```

## 11. TASK-P8-002：实现 AgentTickLog

任务名称：实现内存级 Agent tick 日志。

允许修改：

```text
backend/include/pathfinder/agent/record_types.h
backend/src/agent/record_types.cpp
backend/tests/unit/agent/agent_record_types_test.cpp
```

实现要求：

```text
AgentTickLog::append。
AgentTickLog::size。
AgentTickLog::records。
AgentTickLog::findByRecordId。
AgentTickLog::findByAgentId。
AgentTickLog::findByDecisionId。
AgentTickLog::validateBasic。
append 拒绝重复 record_id。
append 拒绝非法 record。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_record_types --output-on-failure
```

测试要求：

```text
append 合法 record 成功。
append 重复 record_id 失败。
append 非法 record 失败且 size 不变。
findByAgentId 返回该 agent 的记录。
findByDecisionId 能找到对应记录。
empty log validate 通过。
```

## 12. TASK-P8-003：实现 AgentRecordBuilder

任务名称：从 P7 Runtime request/result 构建 AgentTickRecord。

允许新增：

```text
backend/include/pathfinder/agent/record_builder.h
backend/src/agent/record_builder.cpp
backend/tests/unit/agent/agent_record_builder_test.cpp
```

允许修改：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

实现要求：

```text
定义 AgentRecordBuildRequest。
实现 AgentRecordBuilder::build。
从 AgentTickRequest 提取 agent_id、actor_id、tick、random_seed、state_version_before。
从 AgentTickResult 提取 runtime_status、decision、command、pipeline_result、trace。
从 PipelineResult 提取 state_change_ids、event_ids、pipeline_status、error_count。
用 request.input_hash / output_hash 填入 record。
生成稳定 AgentTickRecordId 和 AgentDecisionRecordId。
不调用 AgentRuntime。
不调用 Policy。
不调用 RulePipeline。
不修改 GameState。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_record_builder --output-on-failure
rg -n "AgentRuntime\(|FirstSupportedPolicy|RulePipeline|execute\(|edible_profile|hunger_delta|health_delta|effect_kind" backend/include/pathfinder/agent/record_builder.h backend/src/agent/record_builder.cpp backend/tests/unit/agent/agent_record_builder_test.cpp && exit 1 || true
```

测试要求：

```text
PipelineSucceeded tick_result 生成合法 AgentTickRecord。
SubmitSkipped tick_result 生成合法 AgentTickRecord，但无 replay command 要求。
NoDecision tick_result 生成合法 AgentTickRecord，decision status=NoDecision。
缺 input_hash/output_hash 返回 error。
record_id deterministic。
selected_candidate_id 来自 runtime trace。
```

## 13. TASK-P8-004：实现 AgentReplayBridge

任务名称：把 AgentTickRecord 转成 P4 CommandReplayRecord。

允许新增：

```text
backend/include/pathfinder/agent/replay_bridge.h
backend/src/agent/replay_bridge.cpp
backend/tests/unit/agent/agent_replay_bridge_test.cpp
```

实现要求：

```text
AgentReplayBridge::toCommandReplayRecord。
AgentReplayBridge::appendCommandReplayRecord。
只有 PipelineSucceeded / PipelineFailed 且 command 存在的 AgentTickRecord 能转换。
SubmitSkipped / NoDecision / Failed 返回明确 error。
生成的 CommandReplayRecord 必须 validateBasic 通过。
appendCommandReplayRecord 必须调用 CommandReplayLog::append。
不调用 ReplayRunner。
不调用 RulePipeline。
不修改 GameState。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_replay_bridge --output-on-failure
rg -n "ReplayRunner|RulePipeline|execute\(|GameState|edible_profile|hunger_delta|health_delta|effect_kind" backend/include/pathfinder/agent/replay_bridge.h backend/src/agent/replay_bridge.cpp backend/tests/unit/agent/agent_replay_bridge_test.cpp && exit 1 || true
```

测试要求：

```text
PipelineSucceeded record 转 CommandReplayRecord 成功。
CommandReplayRecord.command_id 等于 record.command.command_id。
CommandReplayRecord.state_change_ids / event_ids 保持一致。
SubmitSkipped record 转换失败。
NoDecision record 转换失败。
append duplicate replay record 返回 error。
```

## 14. TASK-P8-005：实现 ReplayLockChecker

任务名称：实现最小 replay locked 检查，不重新决策。

允许新增或修改：

```text
backend/include/pathfinder/agent/replay_bridge.h
backend/src/agent/replay_bridge.cpp
backend/tests/unit/agent/agent_replay_bridge_test.cpp
```

实现要求：

```text
定义 AgentReplayLockCheckResult。
实现 AgentReplayLockChecker::check。
有 command_id 的 AgentTickRecord，必须能在 CommandReplayLog 找到同 command_id。
找到则 can_replay_without_policy=true。
找不到则 false，并写 reason_key。
SubmitSkipped / NoDecision 没有 command_id 时，只要 AgentTickRecord 合法，也可以解释历史，但不能生成 command replay。
不调用 AgentRuntime。
不调用 Policy。
不调用 ReplayRunner。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_replay_bridge --output-on-failure
rg -n "AgentRuntime|FirstSupportedPolicy|decide\(|tickOne\(" backend/include/pathfinder/agent/replay_bridge.h backend/src/agent/replay_bridge.cpp backend/tests/unit/agent/agent_replay_bridge_test.cpp && exit 1 || true
```

测试要求：

```text
agent record + matching command log -> can_replay_without_policy=true。
agent record + missing command log -> false。
NoDecision record -> has_agent_record=true, has_command_record=false。
SubmitSkipped record -> has_agent_record=true, has_command_record=false。
```

## 15. TASK-P8-006：unknown fruit Agent record + replay 集成测试

任务名称：证明 Agent 历史记录可以生成 CommandReplayLog，并通过 ReplayRunner 回放。

允许新增：

```text
backend/tests/integration/p8/agent_runtime_record_unknown_fruit_replay_test.cpp
```

允许修改：

```text
backend/tests/CMakeLists.txt
```

实现流程：

```text
1. 创建 UnknownFruitFixture 初始状态 state。
2. 拷贝 initial_state_for_replay。
3. 使用 P7 AgentRuntime tickOne 执行 eat。
4. 使用 AgentRecordBuilder 构建 AgentTickRecord。
5. append 到 AgentTickLog。
6. 使用 AgentReplayBridge 转 CommandReplayRecord。
7. append 到 CommandReplayLog。
8. 构造 ReplayInput(initial_state_for_replay, command_log, empty random_log)。
9. 调用 ReplayRunner.replayOne。
10. 断言 ReplayCompareStatus::Match。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R p8_agent_runtime_record_unknown_fruit_replay --output-on-failure
```

强制要求：

```text
Replay 阶段不得创建 AgentRuntime。
Replay 阶段不得调用 Policy。
Replay 阶段只使用 CommandReplayLog + ReplayRunner。
```

## 16. TASK-P8-007：no-submit / no-decision 记录集成测试

任务名称：证明未提交动作也能被解释，但不能伪造成可复盘命令。

允许新增：

```text
backend/tests/integration/p8/agent_runtime_record_no_submit_test.cpp
```

实现流程：

```text
Fire threat -> Runtime SubmitSkipped -> AgentTickRecord 合法 -> AgentReplayBridge 转 CommandReplayRecord 失败。
Social threat -> Runtime NoDecision -> AgentTickRecord 合法 -> AgentReplayBridge 转 CommandReplayRecord 失败。
ReplayLockChecker 对这两类记录返回 has_agent_record=true。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R p8_agent_runtime_record_no_submit --output-on-failure
rg -n "FleeResolver|GroupCombat|WarResolver|tribe_split|pack_combat" backend/include/pathfinder backend/src && exit 1 || true
```

## 17. TASK-P8-008：边界审计和开发笔记

任务名称：更新 Agent 开发笔记并执行 P8 边界审计。

允许修改：

```text
backend/dev_notes/agent.md
doc/review/P8Agent运行记录与复盘兼容审核报告.md
```

审计要求：

```text
确认 AgentRecordBuilder 不调用 AgentRuntime / Policy / RulePipeline。
确认 AgentReplayBridge 不调用 RulePipeline / ReplayRunner。
确认 ReplayRunner 回放阶段不重新调用 Policy。
确认 SubmitSkipped / NoDecision 不生成 CommandReplayRecord。
确认没有新增 SaveManager / H5 / HTTP / WebSocket / RL。
确认没有新增 FleeResolver / GroupCombat / WarResolver。
确认 P4/P7 回归测试通过。
确认 P8 集成测试通过。
```

验收命令：

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent|p8|p7|p6|p5|p4|p3|replay" --output-on-failure
ctest --test-dir build/backend --output-on-failure
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|RulePipeline|execute\(" backend/include/pathfinder/agent/record_builder.h backend/src/agent/record_builder.cpp backend/include/pathfinder/agent/replay_bridge.h backend/src/agent/replay_bridge.cpp && exit 1 || true
rg -n "edible_profile|hunger_delta|health_delta|effect_kind" backend/include/pathfinder/agent backend/src/agent backend/tests/unit/agent backend/tests/integration/p8 && exit 1 || true
rg -n "SaveManager|WebSocket|HTTP|rl_environment|RLPolicy|NeuralPolicy|LLMPolicy|BehaviorTree|GOAP|FleeResolver|GroupCombat|WarResolver|tribe_split|pack_combat" backend/include/pathfinder/agent backend/src/agent backend/tests/integration/p8 && exit 1 || true
find backend -type d \( -path '*/frontend*' -o -path '*/http*' -o -path '*/websocket*' -o -path '*/save*' \) -print
```

审核报告必须包含：

```text
构建结果。
P8 定向测试结果。
P4/P7 回归测试结果。
全量测试结果。
边界扫描结果。
是否允许进入 P9。
```

## 18. P8 设计风险和处理

### 18.1 风险：复盘时重新思考

处理：

```text
Replay 集成测试只允许使用 CommandReplayLog + ReplayRunner。
AgentReplayBridge 不允许调用 AgentRuntime / Policy。
边界扫描禁止 replay bridge 出现 decide / tickOne。
```

### 18.2 风险：把 AgentDecisionRecord 当规则权威

处理：

```text
AgentDecisionRecord 只解释历史。
CommandReplayRecord 才是 ReplayRunner 的命令输入。
RulePipeline 仍是唯一结算入口。
```

### 18.3 风险：提前做 SaveManager

处理：

```text
P8 全部内存级。
不写文件格式。
不写数据库。
不写存档目录。
```

### 18.4 风险：训练系统提前膨胀

处理：

```text
P8 只保证记录字段能派生 Observation/Action/Result/Trace。
不实现 Reward。
不实现 Done。
不实现 batch dataset。
不实现训练环境。
```

### 18.5 风险：记录泄露隐藏真相

处理：

```text
AgentRecordBuilder 只从 AgentTickRequest / AgentTickResult 读取。
不读取 object definition 的真实效果。
不读取 hidden/debug truth。
边界扫描禁止 edible_profile 等字段。
```

## 19. P8 完成后的系统能力

P8 完成后，系统具备：

```text
Agent 每次 tick 可以形成稳定 AgentTickRecord。
Agent 决策可以形成 AgentDecisionRecord。
Agent 记录能关联 CommandReplayRecord。
ReplayRunner 能用 Agent 产生的命令记录回放结果。
回放阶段不重新调用 Policy。
SubmitSkipped / NoDecision 也能被解释，但不会伪造成命令。
后续 P9 可以基于这些记录做 Agent replay lock / training sample / protocol projection。
```

P8 完成后仍不具备：

```text
完整 SaveManager。
文件存档。
前端回放 UI。
训练环境。
Reward/Done 计算。
多 Agent episode。
复杂策略比较。
```

## 20. P8 之后建议

P8 之后建议进入 P9：

```text
P9：Agent 复盘锁定与训练样本前置。
```

P9 建议目标：

```text
明确 ReplayLocked 模式。
把 AgentTickRecord 派生为最小 TrainingSampleDraft。
定义 Observation / Action / Result / Done 的稳定字段。
仍不做 RL 算法，只做数据契约和确定性校验。
```

H5 仍建议放在 Agent Runtime + Replay Record 稳定之后，否则前端会缺少权威事件和历史解释来源。
