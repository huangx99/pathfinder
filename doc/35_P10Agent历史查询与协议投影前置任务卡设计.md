# P10 Agent历史查询与协议投影前置任务卡设计

## 1. P10 要解决什么问题

P8 负责记录 Agent tick，P9 负责 replay lock 和训练样本草稿。P10 要解决的是：

```text
后端已经有 Agent 历史数据，但前端、调试工具、未来训练工具不能直接读取内部结构。
必须先有一层只读查询和安全投影，把内部记录转成稳定、可裁剪、不会泄露隐藏真相的 DTO。
```

P10 的一句话目标：

```text
把 AgentTickRecord / AgentReplayLockSet / AgentTrainingSampleDraft 转成只读历史查询结果和协议投影 DTO。
```

P10 是协议投影前置，不是 H5，不是 HTTP 服务，不是 WebSocket，不是存档系统。

## 2. P10 与前置阶段的关系

P10 依赖：

```text
P4：ProtocolEnvelope 基础结构。
P8：AgentTickRecord / AgentDecisionRecord / AgentTickLog。
P9：AgentReplayLockSet / AgentReplayLockEntry / AgentTrainingSampleDraft。
```

P10 不改变：

```text
规则权威仍是 RulePipeline。
复盘权威仍是 CommandReplayLog + ReplayRunner。
Agent 历史权威仍是 AgentTickRecord。
训练草稿权威仍是 AgentTrainingSampleDraft。
协议投影只是只读视图，不参与结算。
```

## 3. P10 工程定位

P10 分两层：

```text
AgentHistoryQuery 层：按 agent_id / tick / status 查询 Agent 历史。
AgentProtocolProjection 层：把查询结果转成协议层 DTO / ProtocolEnvelope。
```

P10 不做：

```text
不做 H5 页面。
不做 HTTP server。
不做 WebSocket。
不做 SaveManager。
不做数据库。
不做 JSON 序列化器。
不做权限系统。
不做 ReplayRunner 调用。
不做 AgentRuntime 调用。
不做 RL 算法。
不做 Reward / Done 计算。
不做多 Agent 调度。
不做新玩法规则。
```

## 4. P10 最小闭环定义

### 4.1 unknown fruit 历史投影闭环

输入：

```text
P8 unknown fruit AgentTickLog。
P9 AgentReplayLockSet。
P9 AgentTrainingSampleDraft 列表。
```

流程：

```text
AgentHistoryQueryService 按 agent_id 查询历史。
AgentHistoryProjector 合并 tick record / replay lock / training sample summary。
生成 AgentHistoryProjection。
AgentProtocolProjectionAdapter 生成 ProtocolEnvelope。
```

验收：

```text
Projection 包含 agent_id、tick、runtime_status、decision_status、command_id、replay_lock_status、training_sample_status。
Projection 不包含 hidden truth。
Projection 不包含完整 GameState。
Projection 不包含 reward_value / done bool。
ProtocolEnvelope validateBasic 通过。
```

### 4.2 no-decision 历史投影闭环

输入：

```text
P8 NoDecision AgentTickRecord。
P9 ExplainedOnly ReplayLockEntry。
P9 NoDecision TrainingSampleDraft。
```

验收：

```text
Projection runtime_status=NoDecision。
Projection replay_lock_status=ExplainedOnly。
Projection training_sample_status=NoDecision。
Projection 可以包含 reason_key / phase_keys 摘要。
Projection 不伪造 command_id。
Projection 不伪造 replay_record_id。
```

### 4.3 query filter 闭环

输入：

```text
AgentTickLog 中有多个 tick / 多个 agent。
```

验收：

```text
按 agent_id 过滤正确。
按 tick_from / tick_to 过滤正确。
按 sort_order 排序正确。
limit 生效并标记 truncated。
查询不修改输入 log。
```

## 5. P10 新增核心类型

### 5.1 AgentHistoryQueryId

文件建议：`backend/include/pathfinder/agent/history_query.h`

```cpp
struct AgentHistoryQueryIdTag {};
using AgentHistoryQueryId = foundation::StrongId<AgentHistoryQueryIdTag>;
```

ID 规则：

```text
agent_history_query_<agent_id_or_all>_<tick_from>_<tick_to>_<limit>
```

注意：

```text
必须提供 helper 生成。
不得手写随机 ID。
不得使用中文、空格、冒号、斜杠。
```

### 5.2 AgentHistorySortOrder

```cpp
enum class AgentHistorySortOrder {
    Unknown,
    TickAscending,
    TickDescending
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值，非法有效状态 |
| TickAscending | tick 升序 | 从旧到新 |
| TickDescending | tick 降序 | 从新到旧 |

### 5.3 AgentHistoryProjectionMode

```cpp
enum class AgentHistoryProjectionMode {
    Unknown,
    Public,
    Debug,
    Training
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值 |
| Public | 普通展示 | 未来前端默认，只展示安全摘要 |
| Debug | 调试展示 | 可展示 reason_keys / phase_keys 摘要 |
| Training | 训练展示 | 可展示训练样本草稿摘要 |

注意：

```text
三种模式都不能展示 hidden truth。
Debug 也不能展示 ObjectDefinition 真实效果。
Training 也不能展示 reward_value / done bool，因为 P9 尚未计算。
```

### 5.4 AgentHistoryQueryOptions

```cpp
struct AgentHistoryQueryOptions {
    AgentHistoryQueryId query_id;
    std::optional<AgentId> agent_id;
    std::optional<foundation::Tick> tick_from;
    std::optional<foundation::Tick> tick_to;
    AgentHistorySortOrder sort_order = AgentHistorySortOrder::TickAscending;
    AgentHistoryProjectionMode projection_mode = AgentHistoryProjectionMode::Public;
    size_t limit = 100;
    bool include_replay_lock = true;
    bool include_training_sample = true;
    bool include_debug_trace = false;

    foundation::Result<void> validateBasic() const;
};
```

约束：

```text
query_id 必须合法。
sort_order 不能 Unknown。
projection_mode 不能 Unknown。
limit 必须 >0 且 <=1000。
如果 tick_from > tick_to，返回 error。
Public 模式下 include_debug_trace 必须 false。
```

### 5.5 AgentHistoryQueryResult

```cpp
struct AgentHistoryQueryResult {
    AgentHistoryQueryId query_id;
    std::vector<AgentTickRecord> records;
    size_t total_matched = 0;
    bool truncated = false;

    foundation::Result<void> validateBasic() const;
};
```

约束：

```text
records 中每条 AgentTickRecord 必须 validateBasic。
records.size <= total_matched。
truncated=true 表示因为 limit 被裁剪。
```

### 5.6 AgentHistoryQueryService

```cpp
class AgentHistoryQueryService {
public:
    foundation::Result<AgentHistoryQueryResult> query(
        const AgentTickLog& log,
        const AgentHistoryQueryOptions& options) const;
};
```

职责：

```text
只读查询 AgentTickLog。
不修改 AgentTickLog。
不调用 AgentRuntime。
不调用 Policy。
不调用 RulePipeline。
不调用 ReplayRunner。
不读取 GameState。
```

## 6. P10 投影 DTO 类型

文件建议：`backend/include/pathfinder/protocol/agent_projection.h`

### 6.1 AgentHistoryItemProjection

```cpp
struct AgentHistoryItemProjection {
    std::string record_id;
    std::string agent_id;
    std::string actor_id;
    int64_t tick = 0;
    uint64_t state_version_before = 0;
    uint64_t state_version_after = 0;

    std::string runtime_status;
    std::string decision_status;
    std::optional<std::string> selected_candidate_id;
    std::optional<std::string> selected_command_action_id;
    std::optional<std::string> command_id;
    std::optional<std::string> replay_record_id;

    std::optional<std::string> replay_lock_status;
    std::optional<std::string> training_sample_status;

    size_t state_change_count = 0;
    size_t event_count = 0;
    size_t error_count = 0;

    std::vector<std::string> phase_keys;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    foundation::Result<void> validateBasic() const;
};
```

约束：

```text
record_id / agent_id / actor_id 必须非空。
runtime_status / decision_status 必须非空。
Public 模式下 phase_keys / reason_keys / warning_keys 应为空。
NoDecision / SubmitSkipped 在 Debug 模式下必须能展示解释 key。
不得包含 hidden truth。
不得包含完整 CommandEnvelope。
不得包含完整 GameState。
```

### 6.2 AgentHistoryProjection

```cpp
struct AgentHistoryProjection {
    std::string query_id;
    AgentHistoryProjectionMode projection_mode = AgentHistoryProjectionMode::Unknown;
    size_t total_matched = 0;
    bool truncated = false;
    std::vector<AgentHistoryItemProjection> items;

    foundation::Result<void> validateBasic() const;
};
```

### 6.3 AgentReplayLockItemProjection

```cpp
struct AgentReplayLockItemProjection {
    std::string agent_record_id;
    std::string agent_id;
    int64_t tick = 0;
    std::optional<std::string> command_id;
    std::optional<std::string> replay_record_id;
    std::string replay_lock_status;
    std::vector<std::string> reason_keys;

    foundation::Result<void> validateBasic() const;
};
```

### 6.4 AgentReplayLockProjection

```cpp
struct AgentReplayLockProjection {
    std::string lock_set_id;
    bool all_replayable_without_policy = false;
    bool has_broken_entry = false;
    std::vector<AgentReplayLockItemProjection> entries;

    foundation::Result<void> validateBasic() const;
};
```

### 6.5 AgentTrainingSampleProjection

```cpp
struct AgentTrainingSampleProjection {
    std::string sample_id;
    std::string source_record_id;
    std::string status;
    std::string agent_id;
    std::string actor_id;
    int64_t tick = 0;
    std::optional<std::string> command_id;
    std::string runtime_status;
    std::string decision_status;
    std::string reward_status;
    std::string done_status;
    bool replay_locked = false;

    size_t observed_object_count = 0;
    size_t observed_threat_count = 0;
    size_t knowledge_claim_count = 0;
    size_t state_change_count = 0;
    size_t event_count = 0;

    foundation::Result<void> validateBasic() const;
};
```

强制约束：

```text
不得出现 reward_value。
不得出现 done / is_done bool。
done_status 只是字符串状态，不代表已判定结束。
```

## 7. P10 Projector / Adapter

### 7.1 AgentHistoryProjector

文件建议：`backend/include/pathfinder/protocol/agent_projection.h`

```cpp
struct AgentHistoryProjectorInput {
    AgentHistoryQueryResult query_result;
    const AgentReplayLockSet* replay_lock_set = nullptr;
    const std::vector<AgentTrainingSampleDraft>* training_samples = nullptr;
    AgentHistoryProjectionMode projection_mode = AgentHistoryProjectionMode::Public;
    bool include_debug_trace = false;

    foundation::Result<void> validateBasic() const;
};

class AgentHistoryProjector {
public:
    foundation::Result<AgentHistoryProjection> project(
        const AgentHistoryProjectorInput& input) const;
};
```

职责：

```text
把 AgentHistoryQueryResult 转成 AgentHistoryProjection。
按 source_record_id 关联 TrainingSampleDraft。
按 agent_record_id 关联 ReplayLockEntry。
Public 模式隐藏 phase/reason/warning keys。
Debug 模式允许显示 reason/phase/warning keys 摘要。
Training 模式允许显示 training_sample_status 摘要。
不调用 AgentRuntime / Policy / RulePipeline / ReplayRunner。
不读取 GameState / ObjectDefinition / hidden truth。
```

### 7.2 AgentProtocolProjectionAdapter

```cpp
struct AgentProjectionProtocolOptions {
    std::string protocol_version = "1.0";
    std::optional<std::string> request_id;
    std::optional<std::string> correlation_id;
    std::optional<std::string> session_id;
};

class AgentProtocolProjectionAdapter {
public:
    ProtocolEnvelope buildHistoryEnvelope(
        const AgentHistoryProjection& projection,
        const AgentProjectionProtocolOptions& options = {}) const;

    ProtocolEnvelope buildReplayLockEnvelope(
        const AgentReplayLockProjection& projection,
        const AgentProjectionProtocolOptions& options = {}) const;

    ProtocolEnvelope buildTrainingSampleEnvelope(
        const AgentTrainingSampleProjection& projection,
        const AgentProjectionProtocolOptions& options = {}) const;
};
```

实现原则：

```text
P10 可以扩展 ProtocolPayloadType：AgentHistoryProjection / AgentReplayLockProjection / AgentTrainingSampleProjection。
ProtocolEnvelope 只承载 envelope 元信息和 payload 类型。
P10 不实现 JSON 序列化。
P10 不实现网络发送。
P10 不实现前端页面。
```

## 8. 文件范围

### 8.1 允许新增

```text
backend/include/pathfinder/agent/history_query.h
backend/src/agent/history_query.cpp
backend/include/pathfinder/protocol/agent_projection.h
backend/src/protocol/agent_projection.cpp
backend/tests/unit/agent/agent_history_query_test.cpp
backend/tests/unit/protocol/agent_projection_test.cpp
backend/tests/integration/p10/agent_history_projection_flow_test.cpp
backend/tests/integration/p10/agent_no_decision_projection_flow_test.cpp
context_packs/agent_p10.md
```

### 8.2 允许修改

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
backend/include/pathfinder/protocol/types.h
backend/src/protocol/types.cpp
backend/tests/unit/protocol/protocol_smoke_test.cpp
```

说明：

```text
protocol/types 只允许补充 P10 需要的 ProtocolPayloadType 枚举和 roundtrip 测试。
不允许改变既有 CommandResult / ReplayReport 语义。
```

### 8.3 禁止修改

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

## 9. 必读文档

P10 实现 AI 必须读取：

```text
doc/35_P10Agent历史查询与协议投影前置任务卡设计.md
context_packs/agent_p10.md
context_packs/agent_p9.md
context_packs/agent_p8.md
doc/20_前后端协议设计.md 的 envelope / projection / replay 片段
doc/21_Agent系统设计.md 的 AgentDecisionRecord / ReplayLocked / 训练关系片段
backend/dev_notes/agent.md
```

最多再读取：

```text
backend/include/pathfinder/agent/record_types.h
backend/include/pathfinder/agent/replay_lock.h
backend/include/pathfinder/agent/training_sample.h
backend/include/pathfinder/protocol/envelope.h
backend/include/pathfinder/protocol/types.h
```

禁止为了 P10 一次性读取：

```text
01-20 全部文档。
完整规则系统。
完整对象系统。
完整 H5 或 SaveManager 设计。
```

## 10. TASK-P10-000：上下文包

任务名称：创建 P10 上下文包。

允许新增：

```text
context_packs/agent_p10.md
```

必须包含：

```text
P10 目标。
P10 与 P8/P9/P4 的关系。
只读查询原则。
安全投影原则。
不做 H5 / HTTP / WebSocket / SaveManager / JSON 序列化。
文件范围。
验收命令。
```

验收命令：

```bash
rg -n "P10|AgentHistoryQuery|AgentHistoryProjection|ProtocolEnvelope|只读|不做 H5" context_packs/agent_p10.md
```

## 11. TASK-P10-001：实现 AgentHistoryQuery 基础类型

任务名称：实现 Agent 历史查询选项和结果类型。

允许新增：

```text
backend/include/pathfinder/agent/history_query.h
backend/src/agent/history_query.cpp
backend/tests/unit/agent/agent_history_query_test.cpp
```

允许修改：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

实现要求：

```text
定义 AgentHistoryQueryId。
定义 AgentHistorySortOrder。
定义 AgentHistoryProjectionMode。
定义 AgentHistoryQueryOptions。
定义 AgentHistoryQueryResult。
实现 toString/fromString。
实现 validateBasic。
实现 makeAgentHistoryQueryId helper。
不依赖 ProtocolEnvelope。
不依赖 GameState。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_history_query --output-on-failure
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|RulePipeline|execute\(|ReplayRunner|GameState|ObjectDefinition|HTTP|WebSocket|SaveManager" backend/include/pathfinder/agent/history_query.h backend/src/agent/history_query.cpp backend/tests/unit/agent/agent_history_query_test.cpp && exit 1 || true
```

测试要求：

```text
枚举 roundtrip 覆盖全部枚举。
合法 query options 通过。
query_id 为空失败。
sort_order Unknown 失败。
projection_mode Unknown 失败。
limit=0 失败。
limit>1000 失败。
tick_from > tick_to 失败。
Public + include_debug_trace=true 失败。
```

## 12. TASK-P10-002：实现 AgentHistoryQueryService

任务名称：实现只读历史查询。

允许修改：

```text
backend/include/pathfinder/agent/history_query.h
backend/src/agent/history_query.cpp
backend/tests/unit/agent/agent_history_query_test.cpp
```

实现要求：

```text
实现 AgentHistoryQueryService::query。
支持 agent_id 过滤。
支持 tick_from / tick_to 过滤。
支持 TickAscending / TickDescending。
支持 limit 和 truncated。
返回记录必须保持 validateBasic。
不修改 AgentTickLog。
不调用 AgentRuntime / Policy / RulePipeline / ReplayRunner。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_history_query --output-on-failure
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|RulePipeline|execute\(|ReplayRunner" backend/include/pathfinder/agent/history_query.h backend/src/agent/history_query.cpp backend/tests/unit/agent/agent_history_query_test.cpp && exit 1 || true
```

测试要求：

```text
按 agent_id 过滤正确。
按 tick range 过滤正确。
升序排序正确。
降序排序正确。
limit 裁剪并 truncated=true。
无匹配返回空 records 且 total_matched=0。
查询前后原 log size 不变。
```

## 13. TASK-P10-003：扩展 ProtocolPayloadType

任务名称：为 Agent 投影补协议 payload 类型。

允许修改：

```text
backend/include/pathfinder/protocol/types.h
backend/src/protocol/types.cpp
backend/tests/unit/protocol/protocol_smoke_test.cpp
```

新增枚举建议：

```cpp
AgentHistoryProjection,
AgentReplayLockProjection,
AgentTrainingSampleProjection
```

实现要求：

```text
toString/fromString 覆盖新增枚举。
现有枚举字符串不变。
现有 protocol 测试继续通过。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R protocol --output-on-failure
```

## 14. TASK-P10-004：实现 Agent Projection DTO

任务名称：实现 Agent 历史 / replay lock / training sample 安全投影 DTO。

允许新增：

```text
backend/include/pathfinder/protocol/agent_projection.h
backend/src/protocol/agent_projection.cpp
backend/tests/unit/protocol/agent_projection_test.cpp
```

允许修改：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

实现要求：

```text
定义 AgentHistoryItemProjection。
定义 AgentHistoryProjection。
定义 AgentReplayLockItemProjection。
定义 AgentReplayLockProjection。
定义 AgentTrainingSampleProjection。
实现 validateBasic。
不包含完整 GameState。
不包含完整 CommandEnvelope。
不包含 ObjectDefinition。
不包含 hidden truth。
不包含 reward_value。
不包含 done / is_done bool。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_projection --output-on-failure
rg -n "GameState|ObjectDefinition|CommandEnvelope|edible_profile|hunger_delta|health_delta|effect_kind|reward_value|done =|is_done|HTTP|WebSocket|SaveManager" backend/include/pathfinder/protocol/agent_projection.h backend/src/protocol/agent_projection.cpp backend/tests/unit/protocol/agent_projection_test.cpp && exit 1 || true
```

测试要求：

```text
合法 history item 通过。
缺 record_id 失败。
缺 agent_id 失败。
缺 runtime_status 失败。
Public projection 带 debug trace 失败或被 Projector 清空。
training projection reward_status/done_status 必须非空。
training projection 不存在 reward_value/done bool。
```

## 15. TASK-P10-005：实现 AgentHistoryProjector

任务名称：把查询结果投影成安全 DTO。

允许修改：

```text
backend/include/pathfinder/protocol/agent_projection.h
backend/src/protocol/agent_projection.cpp
backend/tests/unit/protocol/agent_projection_test.cpp
```

实现要求：

```text
定义 AgentHistoryProjectorInput。
实现 AgentHistoryProjector::project。
从 AgentTickRecord 填充基础字段。
按 agent_record_id 关联 ReplayLockEntry。
按 source_record_id 关联 TrainingSampleDraft。
Public 模式清空 phase_keys / reason_keys / warning_keys。
Debug 模式允许输出 phase_keys / reason_keys / warning_keys。
Training 模式允许输出 training_sample_status。
不调用 AgentRuntime / Policy / RulePipeline / ReplayRunner。
不读取 GameState / ObjectDefinition / hidden truth。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_projection --output-on-failure
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|RulePipeline|execute\(|ReplayRunner|GameState|ObjectDefinition|edible_profile|hunger_delta|health_delta|effect_kind" backend/include/pathfinder/protocol/agent_projection.h backend/src/protocol/agent_projection.cpp backend/tests/unit/protocol/agent_projection_test.cpp && exit 1 || true
```

测试要求：

```text
PipelineSucceeded record -> projection 包含 command_id。
NoDecision record -> projection 不包含 command_id。
ReplayLockEntry Locked -> replay_lock_status=Locked。
TrainingSampleDraft ReplayLocked -> training_sample_status=ReplayLocked。
Public 模式不输出 trace keys。
Debug 模式输出 trace keys。
```

## 16. TASK-P10-006：实现 Protocol Adapter

任务名称：把 Agent 投影包装成 ProtocolEnvelope。

允许修改：

```text
backend/include/pathfinder/protocol/agent_projection.h
backend/src/protocol/agent_projection.cpp
backend/tests/unit/protocol/agent_projection_test.cpp
```

实现要求：

```text
定义 AgentProjectionProtocolOptions。
实现 AgentProtocolProjectionAdapter。
buildHistoryEnvelope 使用 ProtocolPayloadType::AgentHistoryProjection。
buildReplayLockEnvelope 使用 ProtocolPayloadType::AgentReplayLockProjection。
buildTrainingSampleEnvelope 使用 ProtocolPayloadType::AgentTrainingSampleProjection。
Envelope message_type=Response。
Envelope domain=Query 或 ProjectionSync，保持实现一致并写测试。
Envelope validateBasic 通过。
不实现 JSON。
不发送网络。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R "agent_projection|protocol" --output-on-failure
rg -n "nlohmann|json|asio|httplib|WebSocket|socket|send\(|recv\(" backend/include/pathfinder/protocol/agent_projection.h backend/src/protocol/agent_projection.cpp backend/tests/unit/protocol/agent_projection_test.cpp && exit 1 || true
```

测试要求：

```text
history envelope validateBasic 通过。
replay lock envelope validateBasic 通过。
training sample envelope validateBasic 通过。
request_id / correlation_id / session_id 可透传。
message_id 稳定且合法。
payload_type 正确。
```

## 17. TASK-P10-007：unknown fruit 历史投影集成测试

任务名称：证明 P8/P9 的成功 Agent 历史可查询、投影、包装协议。

允许新增：

```text
backend/tests/integration/p10/agent_history_projection_flow_test.cpp
```

允许修改：

```text
backend/tests/CMakeLists.txt
```

实现流程：

```text
1. 构造或复用 P8 unknown fruit AgentTickRecord。
2. 构造或复用 P9 Locked ReplayLockEntry / AgentReplayLockSet。
3. 构造或复用 P9 ReplayLocked TrainingSampleDraft。
4. AgentHistoryQueryService 查询目标 agent。
5. AgentHistoryProjector 生成 Training 模式 projection。
6. AgentProtocolProjectionAdapter 生成 ProtocolEnvelope。
7. 断言 command_id 存在。
8. 断言 replay_lock_status=Locked。
9. 断言 training_sample_status=ReplayLocked。
10. 断言 envelope validateBasic 通过。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R p10_agent_history_projection_flow --output-on-failure
```

强制要求：

```text
不调用 AgentRuntime。
不调用 Policy。
不调用 RulePipeline。
不调用 ReplayRunner。
不读取 GameState。
```

## 18. TASK-P10-008：NoDecision 历史投影集成测试

任务名称：证明 NoDecision 历史可解释投影，但不伪造命令。

允许新增：

```text
backend/tests/integration/p10/agent_no_decision_projection_flow_test.cpp
```

允许修改：

```text
backend/tests/CMakeLists.txt
```

实现流程：

```text
1. 构造或复用 P8 NoDecision AgentTickRecord。
2. 构造 P9 ExplainedOnly ReplayLockEntry。
3. 构造 P9 NoDecision TrainingSampleDraft。
4. Debug 模式投影。
5. 断言 command_id 为空。
6. 断言 replay_lock_status=ExplainedOnly。
7. 断言 training_sample_status=NoDecision。
8. 断言 reason_keys 或 phase_keys 可见。
9. Public 模式再投影，断言 trace keys 被清空。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R p10_agent_no_decision_projection_flow --output-on-failure
```

## 19. TASK-P10-009：边界审计和开发笔记

任务名称：更新 Agent 开发笔记并执行 P10 审计。

允许修改：

```text
backend/dev_notes/agent.md
doc/review/P10Agent历史查询与协议投影前置审核报告.md
```

审计要求：

```text
确认 QueryService / Projector / ProtocolAdapter 不调用 AgentRuntime / Policy / RulePipeline / ReplayRunner。
确认不读取 GameState / ObjectDefinition / hidden truth。
确认不包含 reward_value / done bool。
确认没有 H5 / HTTP / WebSocket / SaveManager / JSON 序列化。
确认 P8/P9 回归测试通过。
确认 P10 集成测试通过。
```

验收命令：

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent_history_query|agent_projection|p10|protocol" --output-on-failure
ctest --test-dir build/backend -R "agent|p10|p9|p8|p7|p6|p5|p4|p3|replay|protocol" --output-on-failure
ctest --test-dir build/backend --output-on-failure
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|RulePipeline|execute\(|ReplayRunner" backend/include/pathfinder/agent/history_query.h backend/src/agent/history_query.cpp backend/include/pathfinder/protocol/agent_projection.h backend/src/protocol/agent_projection.cpp && exit 1 || true
rg -n "GameState|ObjectDefinition|edible_profile|hunger_delta|health_delta|effect_kind" backend/include/pathfinder/agent/history_query.h backend/src/agent/history_query.cpp backend/include/pathfinder/protocol/agent_projection.h backend/src/protocol/agent_projection.cpp backend/tests/unit/agent/agent_history_query_test.cpp backend/tests/unit/protocol/agent_projection_test.cpp backend/tests/integration/p10 && exit 1 || true
rg -n "reward_value|done =|is_done|RewardCalculator|EpisodeRunner|rl_environment|RLPolicy|NeuralPolicy|LLMPolicy" backend/include/pathfinder/agent backend/include/pathfinder/protocol/agent_projection.h backend/src/agent backend/src/protocol/agent_projection.cpp backend/tests/integration/p10 && exit 1 || true
rg -n "httplib|asio|WebSocket|HTTP server|SaveManager|nlohmann|json|socket|send\(|recv\(" backend/include/pathfinder/agent/history_query.h backend/src/agent/history_query.cpp backend/include/pathfinder/protocol/agent_projection.h backend/src/protocol/agent_projection.cpp backend/tests/integration/p10 && exit 1 || true
find backend -type d \( -path '*/frontend*' -o -path '*/http*' -o -path '*/websocket*' -o -path '*/save*' \) -print
```

审核报告必须包含：

```text
构建结果。
P10 定向测试结果。
P8/P9 回归测试结果。
全量测试结果。
边界扫描结果。
是否允许进入 P11。
```

## 20. P10 设计风险和处理

### 20.1 风险：提前做前端/网络

处理：

```text
P10 只做 C++ DTO 和 ProtocolEnvelope。
不做 H5。
不做 HTTP / WebSocket。
不做 JSON 序列化。
```

### 20.2 风险：投影泄露隐藏真相

处理：

```text
Projector 只读 P8/P9 已有安全记录。
不读 GameState。
不读 ObjectDefinition。
边界扫描禁止 hidden truth 字段。
```

### 20.3 风险：Public 模式暴露调试 trace

处理：

```text
Public 模式必须清空 phase_keys / reason_keys / warning_keys。
Debug 模式才允许输出解释 key。
```

### 20.4 风险：把 TrainingSampleProjection 当最终训练数据

处理：

```text
Projection 只展示 P9 Draft 摘要。
不导出 batch。
不计算 reward。
不判断 done。
```

## 21. P10 完成后的系统能力

P10 完成后，系统具备：

```text
可以按 agent_id / tick range 查询 Agent 历史。
可以把 AgentTickRecord 投影成安全 DTO。
可以把 ReplayLockSet 投影成安全 DTO。
可以把 TrainingSampleDraft 投影成安全 DTO。
可以包装成 ProtocolEnvelope。
可以区分 Public / Debug / Training 投影模式。
未来 H5 和调试工具有稳定只读数据来源。
```

P10 完成后仍不具备：

```text
H5 页面。
HTTP API。
WebSocket 推送。
JSON 序列化。
存档落盘。
权限系统。
真正训练环境。
```

## 22. P10 之后建议

P10 之后建议进入 P11：

```text
P11：只读调试接口或本地查询导出前置。
```

P11 可以二选一：

```text
方案 A：继续后端，做本地 CLI / 文件导出草案，不上网络。
方案 B：开始 H5 前置协议，但仍先不做复杂 UI。
```

当前更建议方案 A，因为 P10 只是 DTO，还没有稳定序列化和导出格式。
