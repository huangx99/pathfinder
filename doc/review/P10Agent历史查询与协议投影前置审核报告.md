# P10 Agent历史查询与协议投影前置审核报告

审核时间：2026-05-17  
审核对象：P10 Agent历史查询与协议投影前置实现  
审核结论：通过，允许进入 P11。

## 1. 审核范围

本次审核依据：

```text
doc/35_P10Agent历史查询与协议投影前置任务卡设计.md
context_packs/agent_p10.md
P8/P9 已通过实现与审核结论
```

本次检查的核心范围：

```text
AgentHistoryQueryService 是否只读查询 AgentTickLog。
AgentHistoryProjector 是否把 P8/P9 内部记录投影为安全 DTO。
AgentProtocolProjectionAdapter 是否只包装 ProtocolEnvelope 元信息。
ProtocolPayloadType 是否只补充 P10 必需枚举。
P10 是否没有引入 H5 / HTTP / WebSocket / SaveManager / JSON / RL / LLM / 新规则结算。
```

## 2. 变更范围核对

新增文件符合 P10 允许范围：

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

修改文件符合 P10 允许范围：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
backend/include/pathfinder/protocol/types.h
backend/src/protocol/types.cpp
backend/tests/unit/protocol/protocol_smoke_test.cpp
```

未发现 P10 禁止范围变更：

```text
未修改 rules / object / pipeline / replay_runner / frontend。
未新增 http / websocket / save 目录。
```

## 3. 功能审核

### 3.1 AgentHistoryQueryService

结果：通过。

已确认能力：

```text
支持 agent_id 过滤。
支持 tick_from / tick_to 过滤。
支持 TickAscending / TickDescending 排序。
支持 limit 裁剪和 truncated 标记。
validateBasic 拒绝空 query_id、Unknown sort_order、Unknown projection_mode、非法 limit、非法 tick range。
Public 模式拒绝 include_debug_trace=true。
查询前后不修改输入 AgentTickLog。
```

边界结论：`AgentHistoryQueryService` 只读取 `AgentTickLog`，没有调用 AgentRuntime / Policy / RulePipeline / ReplayRunner，也没有读取 GameState / ObjectDefinition / 隐藏真相。

### 3.2 AgentHistoryProjector

结果：通过。

已确认能力：

```text
把 AgentTickRecord 投影为 AgentHistoryItemProjection。
关联 AgentReplayLockEntry 输出 replay_lock_status 摘要。
关联 AgentTrainingSampleDraft 输出 training_sample_status 摘要。
Public 模式不输出 phase_keys / reason_keys / warning_keys。
Debug / Training 模式仅在 include_debug_trace=true 时输出调试 keys。
NoDecision 场景不伪造 command_id / replay_record_id。
Projection 仅输出计数和状态摘要，不输出完整 CommandEnvelope / GameState / ObjectDefinition。
```

边界结论：投影层没有泄露 `edible_profile`、`hunger_delta`、`health_delta`、`effect_kind` 等隐藏真相；没有输出 reward_value 或 done bool。

### 3.3 AgentProtocolProjectionAdapter

结果：通过。

已确认能力：

```text
buildHistoryEnvelope 使用 ProtocolPayloadType::AgentHistoryProjection。
buildReplayLockEnvelope 使用 ProtocolPayloadType::AgentReplayLockProjection。
buildTrainingSampleEnvelope 使用 ProtocolPayloadType::AgentTrainingSampleProjection。
Envelope message_type=Response。
Envelope domain 使用 Query / ProjectionSync。
request_id / correlation_id / session_id 可透传。
Envelope validateBasic 通过。
```

说明：P10 设计明确规定 `ProtocolEnvelope` 只承载 envelope 元信息和 payload 类型，不承载 DTO 内容，不实现 JSON 序列化。因此当前 Adapter 返回 envelope 元信息是符合设计的。

### 3.4 ProtocolPayloadType 扩展

结果：通过。

已确认新增：

```text
AgentHistoryProjection -> agent_history_projection
AgentReplayLockProjection -> agent_replay_lock_projection
AgentTrainingSampleProjection -> agent_training_sample_projection
```

roundtrip 测试已覆盖，未改变既有 CommandResult / EventList / ReplayReport / ErrorList / Text 语义。

## 4. 测试结果

构建结果：通过。

```bash
cmake --build build/backend
```

P10 / protocol 定向测试：通过。

```text
100% tests passed, 0 tests failed out of 66
```

P3-P10 / Agent / Replay / Protocol 阶段回归：通过。

```text
100% tests passed, 0 tests failed out of 230
```

全量测试：通过。

```text
100% tests passed, 0 tests failed out of 276
```

## 5. 边界扫描结果

运行时 / 管线 / 复盘调用扫描：通过。

```text
扫描目标：history_query.h/.cpp、agent_projection.h/.cpp
扫描内容：AgentRuntime( / FirstSupportedPolicy / decide( / tickOne( / RulePipeline / execute( / ReplayRunner
结果：无命中
```

隐藏真相扫描：通过。

```text
扫描目标：P10 查询、投影、单测、集成测试
扫描内容：GameState / ObjectDefinition / edible_profile / hunger_delta / health_delta / effect_kind
结果：无命中
```

Reward / Done / RL 扫描：通过。

```text
扫描目标：agent include/src、P10 投影、P10 集成测试
扫描内容：reward_value / done = / is_done / RewardCalculator / EpisodeRunner / rl_environment / RLPolicy / NeuralPolicy / LLMPolicy
结果：无命中
```

网络 / JSON / 存档扫描：通过。

```text
扫描目标：P10 查询、投影、集成测试
扫描内容：httplib / asio / WebSocket / HTTP server / SaveManager / nlohmann / json / socket / send( / recv(
结果：无命中
```

目录越界扫描：通过。

```text
未发现 frontend / http / websocket / save 相关新增目录。
```

## 6. 风险与建议

### 6.1 非阻塞建议：查询结果可在返回前自校验

当前 `AgentHistoryQueryService::query` 构造 `AgentHistoryQueryResult` 后直接返回，测试已覆盖正常路径，且 records 来自已校验构造的 `AgentTickLog`。建议后续如引入更多过滤条件，可在返回前调用一次 `result.validateBasic()`，作为额外防线。

结论：非阻塞，不影响 P10 通过。

### 6.2 非阻塞建议：投影返回前可自校验

当前 `AgentHistoryProjector::project` 构造 projection 后直接返回，单测已覆盖 `validateBasic` 与 public/debug/training 行为。建议后续字段增多时，在返回前调用 `projection.validateBasic()`，避免新字段破坏投影契约。

结论：非阻塞，不影响 P10 通过。

### 6.3 非阻塞建议：注释澄清 query filter 的 status 范围

P10 文档前文有一句“按 agent_id / tick / status 查询 Agent 历史”，但后续任务卡和验收只要求 agent_id、tick range、sort_order、limit。当前实现符合任务卡细则。建议后续如果需要按 runtime_status / decision_status 过滤，可另设 P 阶段扩展，不应在 P10 临时补范围。

结论：非阻塞，不影响 P10 通过。

## 7. 最终结论

P10 审核通过。

理由：

```text
实现范围符合 P10 任务卡。
查询层保持只读。
投影层没有泄露隐藏真相。
协议层没有提前引入 JSON / 网络 / 前端。
没有调用 AgentRuntime / Policy / RulePipeline / ReplayRunner。
没有引入 reward_value / done bool / RL / LLM。
P10 定向测试、阶段回归、全量测试全部通过。
```

允许进入：P11 Agent本地调试报告与导出前置。
