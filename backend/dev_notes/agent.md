# Agent 模块开发笔记 (P5 + P6 + P7 + P8)

## 模块职责

Agent 模块负责定义智能体的数据契约、命令适配器、观察/动作空间构建器、运行时调度、以及记录与复盘桥接。
P5 做最小闭环，P6 引入 ObservationBuilder 和 ActionSpaceBuilder，P7 引入 AgentRuntime，P8 引入记录与复盘兼容。

## 目录

- `backend/include/pathfinder/agent/` - 公开头文件
- `backend/src/agent/` - 实现
- `backend/tests/unit/agent/` - 单元测试
- `backend/tests/integration/p5/` - P5 集成测试

## 边界

### 允许依赖

- foundation (StrongId, Result, ErrorDetail, Tick, StateVersion, DecisionId, TargetId)
- command (CommandEnvelope, ActionCommand, ActionTarget, CommandSource, CommandIntent)

### 禁止依赖

- pipeline (AgentCommandAdapter 不调用 RulePipeline)
- rules (Agent 不调用 Resolver)
- state (Agent 不直接读写 GameState)
- event (Agent 不直接生成 EventRecord)
- frontend / HTTP / WebSocket

## 核心类型

- AgentId / AgentDefinitionId - 模块内 StrongId 别名
- AgentScale - 智能体尺度枚举
- AgentCognitionBand - 认知层级枚举
- AgentEmbodiment - 具身形式枚举
- AgentControllerType - 控制器类型枚举
- AgentControlAuthority - 控制权限枚举
- AgentIntentType - 意图类型枚举
- AgentDefinition - 智能体定义
- AgentProfile - 智能体画像
- AgentBinding - 智能体绑定
- AgentObservation - 智能体观察
- AgentObservedObject / AgentObservedThreat / AgentObservedKnowledge - 观察子结构
- AgentActionSpace / AgentActionCandidate - 动作空间
- AgentIntent / AgentDecision - 意图与决策
- AgentCommandAdapter - 意图转命令适配器

## 测试入口

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R agent --output-on-failure
```

## P5 任务进度

- [ ] TASK-P5-000: 上下文包
- [ ] TASK-P5-001: 模块骨架
- [ ] TASK-P5-002: 基础枚举
- [ ] TASK-P5-003: AgentDefinition / AgentProfile
- [ ] TASK-P5-004: AgentBinding
- [ ] TASK-P5-005: AgentObservation
- [ ] TASK-P5-006: AgentActionSpace
- [ ] TASK-P5-007: AgentIntent / AgentDecision
- [ ] TASK-P5-008: AgentCommandAdapter
- [ ] TASK-P5-009: unknown fruit Agent 集成测试
- [ ] TASK-P5-010: spider flee fire 测试
- [ ] TASK-P5-011: wolf call pack 测试
- [ ] TASK-P5-012: 边界审计

## P6: ObservationBuilder & ActionSpaceBuilder

### 新增职责

- `ObservationBuilder`: GameState + VisibilityInput → AgentObservation（只读，不修改状态，不调用游戏逻辑）
- `ActionSpaceBuilder`: AgentObservation → AgentActionSpace（只读观察，不访问世界内部）
- `builder_types`: ObservedThreatSeed、VisibilityInput、ObservationBuildRequest 等请求/结果类型

### 目录

- `backend/include/pathfinder/agent/builder_types.h` - Builder 请求/结果类型
- `backend/include/pathfinder/agent/observation_builder.h` - ObservationBuilder
- `backend/include/pathfinder/agent/action_space_builder.h` - ActionSpaceBuilder
- `backend/src/agent/builder_types.cpp` - 类型校验实现
- `backend/src/agent/observation_builder.cpp` - 观察构建实现
- `backend/src/agent/action_space_builder.cpp` - 动作空间构建实现
- `backend/tests/unit/agent/agent_builder_*_test.cpp` - Builder 单元测试
- `backend/tests/integration/p6/` - P6 集成测试

### 允许依赖 (pathfinder_agent_builders)

- pathfinder_agent (P5 类型)
- pathfinder_state (GameState 只读)
- pathfinder_object (WorldObject, ObjectRuntimeFlag)
- pathfinder_cognition (CognitionState 语义查询)
- pathfinder_foundation

### 禁止依赖

- pipeline / rules (Builder 不执行游戏逻辑)
- AgentRuntime / AgentPolicy / RL
- HTTP / WebSocket / SaveManager
- 真实逃跑/群体战斗结算

### 边界扫描命令

```bash
# 扫描1: Builder 代码不引用游戏逻辑
rg -n "RulePipeline|EatObjectResolver|StateChange|EventRecord" \
  backend/include/pathfinder/agent/observation_builder.h \
  backend/src/agent/observation_builder.cpp \
  backend/include/pathfinder/agent/action_space_builder.h \
  backend/src/agent/action_space_builder.cpp

# 扫描2: ActionSpaceBuilder 不引用 GameState
rg -n "GameState|state::" \
  backend/include/pathfinder/agent/action_space_builder.h \
  backend/src/agent/action_space_builder.cpp

# 扫描3: 不暴露真实效果数据
rg -n "edible_profile|hunger_delta|health_delta|effect_kind" \
  backend/include/pathfinder/agent \
  backend/src/agent \
  backend/tests/unit/agent \
  backend/tests/integration/p6
```

### 测试入口

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent_builder|p6" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

## P7: AgentRuntime 最小调度闭环

### 新增职责

- `AgentPolicy` / `FirstSupportedPolicy`: 最小策略接口，从 action_space 选择第一个 command_supported=true 的候选
- `AgentRuntime`: 编排 observe → decide → adapt → submit 流程
- `AgentRuntimeStatus` / `AgentTickRequest` / `AgentTickResult`: 运行时类型
- `AgentActionCandidate.command_action_id` / `suggested_targets`: 语义 action id 和预构建目标

### 目录

- `backend/include/pathfinder/agent/policy.h` - Policy 接口和 FirstSupportedPolicy
- `backend/include/pathfinder/agent/runtime_types.h` - Runtime 请求/结果/状态类型
- `backend/include/pathfinder/agent/runtime.h` - AgentRuntime
- `backend/src/agent/policy.cpp` - Policy 实现
- `backend/src/agent/runtime_types.cpp` - Runtime 类型实现
- `backend/src/agent/runtime.cpp` - Runtime 实现
- `backend/tests/unit/agent/agent_policy_test.cpp` - Policy 单元测试
- `backend/tests/unit/agent/agent_runtime_types_test.cpp` - Runtime 类型测试
- `backend/tests/unit/agent/agent_runtime_test.cpp` - Runtime 单元测试
- `backend/tests/integration/p7/` - P7 集成测试

### 允许依赖 (pathfinder_agent_runtime)

- pathfinder_agent (P5 类型 + P6 builders)
- pathfinder_pipeline (RulePipeline, PipelineContext, PipelineResult)
- pathfinder_state (GameState)
- pathfinder_object (WorldObject)
- pathfinder_cognition (CognitionState)
- pathfinder_command (CommandEnvelope)
- pathfinder_foundation

### 禁止依赖

- AgentService / AgentStore / AgentGoal
- BehaviorTree / GOAP / UtilityPolicy / NeuralPolicy / LLMPolicy / RLPolicy
- FleeResolver / GroupCombat / WarResolver / tribe_split
- H5 / HTTP / WebSocket / SaveManager
- Policy 不读取 GameState
- Runtime 不直接修改 GameState

### 边界扫描命令

```bash
# 扫描1: Policy 不引用 GameState
rg -n "GameState|object_store|actor_store|RulePipeline|EatObjectResolver|edible_profile|hunger_delta|health_delta|effect_kind" \
  backend/include/pathfinder/agent/policy.h \
  backend/src/agent/policy.cpp \
  backend/tests/unit/agent/agent_policy_test.cpp

# 扫描2: 不暴露真实效果数据
rg -n "edible_profile|hunger_delta|health_delta|effect_kind" \
  backend/include/pathfinder/agent \
  backend/src/agent \
  backend/tests/unit/agent \
  backend/tests/integration/p7

# 扫描3: 不引入禁止系统
rg -n "FleeResolver|GroupCombat|WarResolver|tribe_split|pack_combat|rl_environment|BehaviorTree|GOAP|UtilityPolicy|NeuralPolicy|LLMPolicy|WebSocket|HTTP|SaveManager" \
  backend/include/pathfinder/agent \
  backend/src/agent \
  backend/tests/integration/p7
```

### 测试入口

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent_policy|agent_runtime_types|agent_runtime|p7" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

## P8: Agent Runtime Record & Replay Bridge

### 新增职责

- `AgentTickRecord` / `AgentDecisionRecord` / `AgentTickLog`: Agent tick/decision 的内存级记录
- `AgentRecordBuilder`: 从 AgentTickRequest + AgentTickResult 生成稳定 AgentTickRecord
- `AgentReplayBridge`: 将 AgentTickRecord 转换为 P4 CommandReplayRecord
- `AgentReplayLockChecker`: 验证记录与命令日志的回放锁定状态

### 目录

- `backend/include/pathfinder/agent/record_types.h` - 记录类型定义
- `backend/include/pathfinder/agent/record_builder.h` - AgentRecordBuilder
- `backend/include/pathfinder/agent/replay_bridge.h` - AgentReplayBridge + LockChecker
- `backend/src/agent/record_types.cpp` - 记录类型实现
- `backend/src/agent/record_builder.cpp` - AgentRecordBuilder 实现
- `backend/src/agent/replay_bridge.cpp` - ReplayBridge 实现
- `backend/tests/unit/agent/agent_record_types_test.cpp` - 记录类型单元测试
- `backend/tests/unit/agent/agent_record_builder_test.cpp` - Builder 单元测试
- `backend/tests/unit/agent/agent_replay_bridge_test.cpp` - Bridge 单元测试
- `backend/tests/integration/p8/` - P8 集成测试

### 允许依赖 (pathfinder_agent_record)

- pathfinder_agent (P5 类型 + P6 builders + P7 runtime)
- pathfinder_replay (CommandReplayRecord, CommandReplayLog, ReplayRunner)
- pathfinder_pipeline (PipelineResult, PipelineStatus)
- pathfinder_state (StateChange, StateChangeSet)
- pathfinder_event (EventRecord, EventStream)
- pathfinder_command (CommandEnvelope)
- pathfinder_foundation

### 禁止依赖

- AgentRuntime / Policy / RulePipeline (record_builder 不调用运行时)
- ReplayRunner (replay_bridge 不执行回放)
- SaveManager / H5 / HTTP / WebSocket
- FleeResolver / GroupCombat / WarResolver
- edible_profile / hunger_delta / health_delta / effect_kind

### 边界扫描命令

```bash
# 扫描1: record_builder/replay_bridge 不调用运行时
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|RulePipeline|execute\(" \
  backend/include/pathfinder/agent/record_builder.h \
  backend/src/agent/record_builder.cpp \
  backend/include/pathfinder/agent/replay_bridge.h \
  backend/src/agent/replay_bridge.cpp

# 扫描2: 不暴露真实效果数据
rg -n "edible_profile|hunger_delta|health_delta|effect_kind" \
  backend/include/pathfinder/agent \
  backend/src/agent \
  backend/tests/unit/agent \
  backend/tests/integration/p8

# 扫描3: 不引入禁止系统
rg -n "SaveManager|WebSocket|HTTP|rl_environment|RLPolicy|NeuralPolicy|LLMPolicy|BehaviorTree|GOAP|FleeResolver|GroupCombat|WarResolver|tribe_split|pack_combat" \
  backend/include/pathfinder/agent \
  backend/src/agent \
  backend/tests/integration/p8
```

### 测试入口

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent_record|agent_replay_bridge|p8" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

### P8 测试结果 (2026-05-17)

- 171/171 全量测试通过
- 边界扫描全部通过
- 状态版本修复: state_version_after 从 pipeline 执行后捕获，不再使用计算值

---

## P9: Agent 复盘锁定与训练样本前置 (2026-05-17)

### 目标

P9 在 P8 基础上增加两个轻量契约层:

1. **ReplayLock 层**: 判断 AgentTickRecord + CommandReplayLog 是否已锁定，可安全回放且不重新决策
2. **TrainingDraft 层**: 从 AgentTickRecord 派生最小训练样本草稿，P9 不实现训练算法

### 新增文件

```
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

### 核心类型

**ReplayLock 类型:**
- `AgentReplayLockStatus`: Unknown/Locked/ExplainedOnly/Broken/Invalid
- `AgentReplayLockReason`: 命令匹配/不期望命令/命令缺失/ID不匹配/记录非法
- `AgentReplayLockEntry`: 单条锁定记录
- `AgentReplayLockSet`: 锁定集合，支持 allReplayableWithoutPolicy/hasBrokenEntry
- `AgentReplayLockSetBuilder`: 从 AgentTickLog + CommandReplayLog 构建

**TrainingSample 类型:**
- `AgentTrainingSampleStatus`: Unknown/Draft/ReplayLocked/Skipped/NoDecision/Invalid
- `AgentRewardDraftStatus`: Unknown/NotComputed/PlaceholderOnly
- `AgentDoneDraftStatus`: Unknown/NotComputed/PlaceholderOnly
- `AgentObservationSampleDraft/AgentActionSampleDraft/AgentResultSampleDraft/AgentTraceSampleDraft`
- `AgentTrainingSampleDraft`: 完整训练样本草稿
- `AgentTrainingSampleDraftBuilder`: 从 AgentTickRecord 派生

### 设计约束

- ReplayLockSetBuilder 不调用 AgentRuntime/Policy/ReplayRunner/RulePipeline
- TrainingSampleDraftBuilder 不调用 AgentRuntime/Policy/ReplayRunner/RulePipeline
- TrainingSampleDraft 不包含 reward_value/done bool
- 不包含 RewardCalculator/EpisodeRunner
- 不读取 GameState/ObjectDefinition/hidden truth

### 测试入口

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent_replay_lock|agent_training_sample|p9" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

### P9 测试结果 (2026-05-17)

- 229/229 全量测试通过 (P8 171 + P9 58)
- P9 定向测试: 55/55 通过
- 边界扫描全部通过
- P8 回归测试全部通过
- 审核修复: 移除集成测试中 GameState 直接使用，修正 trace 字段映射，补充 source_hash/replay_locked 校验

---

## P10: Agent 历史查询与协议投影前置

### 目标

P10 在 P8/P9 基础上增加只读查询和安全投影层:

1. **AgentHistoryQueryService**: 按 agent_id / tick range 查询 AgentTickLog
2. **AgentHistoryProjector**: 把查询结果投影成安全 DTO
3. **AgentProtocolProjectionAdapter**: 把投影包装成 ProtocolEnvelope

### 新增文件

```
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

### 核心类型

**History Query 类型:**
- `AgentHistoryQueryId`: 查询 ID (StrongId)
- `AgentHistorySortOrder`: Unknown/TickAscending/TickDescending
- `AgentHistoryProjectionMode`: Unknown/Public/Debug/Training
- `AgentHistoryQueryOptions`: 查询选项 (agent_id/tick range/sort/limit/mode)
- `AgentHistoryQueryResult`: 查询结果 (records/total_matched/truncated)
- `AgentHistoryQueryService`: 只读查询服务

**Projection DTO:**
- `AgentHistoryItemProjection`: 单条历史投影
- `AgentHistoryProjection`: 历史投影集合
- `AgentReplayLockItemProjection`: 单条锁定投影
- `AgentReplayLockProjection`: 锁定投影集合
- `AgentTrainingSampleProjection`: 训练样本投影
- `AgentHistoryProjector`: 投影器 (按 mode 过滤)
- `AgentProtocolProjectionAdapter`: 协议适配器

**ProtocolPayloadType 扩展:**
- `AgentHistoryProjection`
- `AgentReplayLockProjection`
- `AgentTrainingSampleProjection`

### 设计约束

- QueryService/Projector/Adapter 不调用 AgentRuntime/Policy/RulePipeline/ReplayRunner
- 不读取 GameState/ObjectDefinition/hidden truth
- 不包含 reward_value/done bool
- Public 模式清空 phase_keys/reason_keys/warning_keys
- Debug 模式允许输出 trace keys
- 不做 H5/HTTP/WebSocket/JSON 序列化

### 测试入口

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent_history_query|agent_projection|p10|protocol" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

### P10 测试结果 (2026-05-17)

- 276/276 全量测试通过 (P9 229 + P10 47)
- P10 定向测试: 66/66 通过 (19 history_query + 24 projection + 4 integration + 19 protocol)
- 边界扫描全部通过
- P8/P9 回归测试全部通过

---

## P11: Agent 本地调试报告与导出前置

### 目标

P11 在 P10 基础上将只读投影整理成可读的本地调试报告和内存级导出片段:

1. **AgentDebugReport**: 从 AgentHistoryProjection 生成结构化调试报告
2. **AgentDiagnosticsSummary**: 从 DebugReport 生成失败/风险/边界摘要
3. **AgentExportDraft**: 从 Report + Diagnostics 生成内存级导出清单和片段

### 新增文件

```
backend/include/pathfinder/agent/debug_report.h
backend/include/pathfinder/agent/debug_export.h
backend/src/agent/debug_report.cpp
backend/src/agent/debug_export.cpp
backend/tests/unit/agent/agent_debug_report_test.cpp
backend/tests/unit/agent/agent_debug_export_test.cpp
backend/tests/integration/p11/agent_debug_report_export_flow_test.cpp
backend/tests/integration/p11/agent_no_decision_debug_report_test.cpp
context_packs/agent_p11.md
```

### 核心类型

**DebugReport 类型:**
- `AgentDebugReportId`: 报告 ID (StrongId)
- `AgentDebugReportMode`: Unknown/Public/Debug/Training
- `AgentDebugReportSeverity`: Unknown/Info/Warning/Error
- `AgentDebugReportItem`: 单条报告项
- `AgentDebugReport`: 完整调试报告
- `AgentDebugReportBuildRequest`: 构建请求
- `AgentDebugReportBuilder`: 从 AgentHistoryProjection 构建

**Diagnostics 类型:**
- `AgentDiagnosticsStatus`: Unknown/Clean/HasWarnings/HasErrors
- `AgentDiagnosticsIssue`: 单条诊断问题
- `AgentDiagnosticsSummary`: 诊断摘要
- `AgentDiagnosticsBuilder`: 从 DebugReport 构建

**ExportDraft 类型:**
- `AgentExportFormat`: Unknown/PlainText/MarkdownLike/ProtocolText
- `AgentExportChunk`: 内存级导出片段
- `AgentExportManifest`: 导出清单
- `AgentExportDraft`: 完整导出草稿
- `AgentExportDraftBuildRequest`: 构建请求
- `AgentExportDraftBuilder`: 从 Report + Diagnostics 构建

### 设计约束

- DebugReportBuilder/DiagnosticsBuilder/ExportDraftBuilder 不调用 AgentRuntime/Policy/RulePipeline/ReplayRunner
- 不读取 AgentTickRecord 原始结构
- 不读取 GameState/ObjectDefinition/hidden truth
- 不包含 reward_value/done bool
- Public 模式清空 reason_keys/phase_keys/warning_keys
- 不写文件、不访问 filesystem
- 不做 JSON/HTTP/WebSocket/H5/SaveManager

### 测试入口

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent_debug_report|agent_debug_export|p11" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

### P11 测试结果 (2026-05-17)

- 332/332 全量测试通过 (P10 276 + P11 56)
- P11 定向测试: 56/56 通过 (35 debug_report + 17 debug_export + 4 integration)
- 边界扫描全部通过
- P8/P9/P10 回归测试全部通过

---

## P12: Agent 本地文件导出与校验

### 目标

P12 在 P11 基础上将内存级导出草稿写入本地文件并校验:

1. **AgentExportPathPolicy**: 路径安全策略 (root_dir/文件名/大小限制)
2. **AgentExportWritePlanner**: 从 AgentExportDraft 生成确定性写入计划
3. **AgentLocalExportService**: 执行本地文件写入 (std::filesystem + std::ofstream)
4. **AgentExportVerifier**: 校验文件数量/大小/路径/内容边界

### 新增文件

```
backend/include/pathfinder/agent/local_export.h
backend/src/agent/local_export.cpp
backend/tests/unit/agent/agent_local_export_test.cpp
backend/tests/integration/p12/agent_local_export_flow_test.cpp
backend/tests/integration/p12/agent_local_export_security_test.cpp
context_packs/agent_p12.md
```

### 核心类型

**ID 类型:**
- `AgentExportFileId`: 导出文件 ID
- `AgentExportWriteId`: 写入任务 ID
- `AgentExportVerifyId`: 校验任务 ID

**枚举:**
- `AgentExportFileKind`: Unknown/Manifest/Chunk/Diagnostics
- `AgentExportFileExtension`: Unknown/Txt/Md
- `AgentExportWriteStatus`: Unknown/Planned/Written/Failed/Skipped
- `AgentExportVerificationStatus`: Unknown/Passed/Failed/Warning

**数据契约:**
- `AgentExportPathPolicy`: 路径策略 (root_dir/allow_create/allow_overwrite/大小限制)
- `AgentExportFilePlan`: 单文件写入计划
- `AgentExportWritePlan`: 完整写入计划
- `AgentExportFileWriteResult`: 单文件写入结果
- `AgentExportWriteResult`: 完整写入结果
- `AgentExportVerificationReport`: 校验报告

**服务:**
- `AgentExportWritePlanner`: AgentExportDraft → AgentExportWritePlan
- `AgentLocalExportService`: 执行写入
- `AgentExportVerifier`: 校验写入结果

### 设计约束

- 使用 std::filesystem 创建目录
- 使用 std::ofstream 写文件
- 不调用 AgentRuntime/Policy/RulePipeline/ReplayRunner
- 不读取 GameState/ObjectDefinition/hidden truth
- 不做 CLI/JSON/HTTP/WebSocket/H5/SaveManager
- 路径穿越被拒绝 (relative_path 不允许 ..)
- 禁止词扫描: GameState/ObjectDefinition/edible_profile/hunger_delta/health_delta/effect_kind/reward_value/is_done/done =

### 测试入口

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent_local_export|p12" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

### P12 测试结果 (2026-05-17)

- 393/393 全量测试通过 (P11 332 + P12 61)
- P12 定向测试: 61/61 通过 (57 unit + 2 integration flow + 2 integration security)
- 边界扫描全部通过
- P8/P9/P10/P11 回归测试全部通过

---

## P13: Agent 本地调试 CLI 前置

### 目标

P13 在 P12 基础上增加 CLI Shell，将 P11/P12 的报告-导出-校验流程暴露为命令行工具:

1. **AgentDebugCliParser**: 解析 argc/argv 参数
2. **AgentDebugFixtureFactory**: 内置 fixture 数据 (UnknownFruit/NoDecision/PublicSafe)
3. **AgentDebugCliRunner**: 编排 validate → fixture → draft → plan → write → verify 流程
4. **CLI main**: 独立可执行文件入口

### 新增文件

```
backend/include/pathfinder/agent/debug_cli.h
backend/src/agent/debug_cli.cpp
backend/tools/agent_debug_cli_main.cpp
backend/tests/unit/agent/agent_debug_cli_test.cpp
backend/tests/integration/p13/agent_debug_cli_flow_test.cpp
backend/tests/integration/p13/agent_debug_cli_security_test.cpp
context_packs/agent_p13.md
```

### 核心类型

**CLI 枚举:**
- `AgentDebugCliCommand`: Unknown/Help/Export
- `AgentDebugCliFixture`: Unknown/UnknownFruit/NoDecision/PublicSafe
- `AgentDebugCliFormat`: Unknown/Text/Markdown/ProtocolText
- `AgentDebugCliExitCode`: Success=0/InvalidArguments=2/BuildFailed=3/WriteFailed=4/VerificationFailed=5/InternalError=10

**数据契约:**
- `AgentDebugCliOptions`: 解析后的 CLI 选项
- `AgentDebugCliResult`: 执行结果 (exit_code/summary_text/output_files)
- `AgentDebugCliParseResult`: 解析结果 (options + result + should_execute)
- `AgentDebugFixtureBundle`: fixture 数据包 (report + diagnostics)

**服务:**
- `AgentDebugCliParser`: argc/argv → AgentDebugCliParseResult
- `AgentDebugFixtureFactory`: fixture 枚举 → AgentDebugFixtureBundle
- `AgentDebugCliRunner`: AgentDebugCliOptions → AgentDebugCliResult

### 设计约束

- 不读取真实 GameState/AgentDefinition/SaveManager
- 内置 fixture 替代真实数据
- 不做 JSON/HTTP/WebSocket
- 解析错误返回 Result::ok() + exit_code=InvalidArguments
- base_name 仅允许 [a-zA-Z0-9_-]
- dry-run 不写文件
- overwrite=false 时文件冲突返回 WriteFailed
- 输出路径均为相对路径

### 测试入口

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent_debug_cli|p13" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

### P13 测试结果 (2026-05-17)

- 447/447 全量测试通过
- P13 定向测试: 54/54 通过
- P3-P13/agent/replay/protocol 相关回归: 401/401 通过
- 边界扫描全部通过
- P13 修复: 输出可执行文件名为 `pathfinder_agent_debug_cli`
- P13 修复: `--scan-content` / `--no-scan-content` 已作为非法参数拒绝，CLI 不再允许关闭内容安全扫描
- P13 修复: 正常执行路径不再额外打印 `Parse OK`
- 历史修复: 2 个 SEGFAULT 由测试中 argc 与 argv 数组大小不匹配导致 (argc=9 但 argv 仅 8 个元素)

---

## P14: Agent 调试输入适配前置

### 目标

P14 在 P13 基础上定义安全输入适配层，让 CLI/调试工具可以接入经过验证的安全 DTO，而不直接读取 GameState、存档内部结构、隐藏真相或原始运行记录:

1. **AgentDebugInputValidator**: 校验输入包的合法性、信任等级、能力匹配、隐藏真相扫描
2. **AgentDebugInputAdapter**: 把已校验输入转换成标准 AgentDebugInputBundle
3. **AgentDebugInputFactory**: 提供稳定构造函数，避免手写 manifest 和 ID

### 新增文件

```
backend/include/pathfinder/agent/debug_input.h
backend/src/agent/debug_input.cpp
backend/tests/unit/agent/agent_debug_input_test.cpp
backend/tests/integration/p14/agent_debug_input_flow_test.cpp
backend/tests/integration/p14/agent_debug_input_security_test.cpp
```

### 核心类型

**枚举:**
- `AgentDebugInputKind`: Unknown/BuiltinFixtureBundle/HistoryProjectionBundle/DebugReportBundle/ExportDraftBundle/InMemoryTestBundle
- `AgentDebugInputTrustLevel`: Unknown/Builtin/GeneratedSafeDto/TestOnly/ExternalUntrusted
- `AgentDebugInputValidationStatus`: Unknown/Valid/ValidWithWarnings/Invalid
- `AgentDebugInputCapability`: Unknown/CanBuildDebugReport/HasDiagnosticsSummary/HasExportDraft/CanWriteThroughP12/TestOnly
- `AgentDebugInputRejectReason`: 16 种拒绝原因

**ID 类型:**
- `AgentDebugInputId`: 输入包 ID (StrongId)

**数据契约:**
- `AgentDebugInputManifest`: 输入清单 (kind/trust_level/capabilities/schema_version)
- `AgentDebugInputSource`: 输入来源 (互斥 payload)
- `AgentDebugInputValidationIssue`: 校验问题
- `AgentDebugInputValidationReport`: 校验报告
- `AgentDebugInputAdapterOptions`: 适配选项 (allow_test_only/allow_export_draft_only/max_items)
- `AgentDebugInputBundle`: 标准调试输入包

**服务:**
- `AgentDebugInputValidator`: 只做校验，不做转换
- `AgentDebugInputAdapter`: validate → adapt → bundle
- `AgentDebugInputFactory`: fromFixture/fromProjection/fromReport/fromExportDraft/fromTestReport

### 设计约束

- 输入先校验，再适配 (validator → adapter)
- 调试输入只认安全 DTO，不读取 GameState/ObjectDefinition/AgentTickRecord
- 隐藏真相扫描: edible_profile/hunger_delta/health_delta/effect_kind/reward_value/done =/is_done/GameState/ObjectDefinition/AgentTickRecord/SaveGame/SaveManager
- ExternalUntrusted 默认拒绝
- ExportDraftBundle 不能反推 report
- 不调用 AgentRuntime/Policy/RulePipeline/ReplayRunner
- 不做 JSON/HTTP/WebSocket/H5/SaveManager

### 测试入口

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent_debug_input|p14" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

### P14 测试结果 (2026-05-17)

- 489/489 全量测试通过 (P13 447 + P14 42)
- P14 定向测试: 42/42 通过 (27 unit + 5 integration flow + 9 integration security + 1 bundle)
- 边界扫描全部通过
- P3-P13/agent/replay/protocol 相关回归: 443/443 通过
- 隐藏真相扫描: 黑名单声明与安全测试允许出现，业务 DTO 中不允许


## P15 认知系统正式化 (Cognition V2)

### 新增文件

- `backend/include/pathfinder/cognition/cognition_v2_types.h` - V2 枚举与 DTO
- `backend/src/cognition/cognition_v2_types.cpp` - V2 类型实现
- `backend/include/pathfinder/cognition/cognition_v2_state.h` - CognitionStateV2
- `backend/src/cognition/cognition_v2_state.cpp` - 状态实现
- `backend/include/pathfinder/cognition/cognition_confidence.h` - 置信模型与更新器
- `backend/src/cognition/cognition_confidence.cpp` - 置信模型实现
- `backend/include/pathfinder/cognition/cognition_query.h` - 查询服务
- `backend/src/cognition/cognition_query.cpp` - 查询实现
- `backend/include/pathfinder/cognition/cognition_evidence_builder.h` - 证据构造器
- `backend/src/cognition/cognition_evidence_builder.cpp` - 证据构造实现

### 修改文件

- `backend/include/pathfinder/state/game_state.h` - 新增 `cognition_state_v2`
- `backend/src/state/game_state.cpp` - 验证 `cognition_state_v2`
- `backend/src/pipeline/rule_pipeline.cpp` - P3 legacy 桥接 V2 evidence
- `backend/CMakeLists.txt` - 添加 V2 源文件到 pathfinder_cognition
- `backend/tests/CMakeLists.txt` - 新增 V2 单元测试与 P15 集成测试

### 测试

- 单元测试: `cognition_v2_types`, `cognition_confidence_model`, `cognition_query_service`, `cognition_v2_update`
- 集成测试: `p15_eat_feedback_flow`, `p15_use_feedback_flow`, `p15_hidden_truth_boundary`

### 边界

- 不读取 ObjectDefinition hidden truth
- 不实现记忆、知识、传播
- 不实现颜色、形状、视觉 reveal
- P3 legacy 认知保持不变



---

## P16: 记忆系统基础 (Memory System Foundation)

### 新增文件

- `backend/include/pathfinder/memory/types.h` - 记忆系统枚举与辅助函数
- `backend/src/memory/types.cpp` - 枚举实现与隐藏真值防护
- `backend/include/pathfinder/memory/memory_record.h` - 核心 DTO (MemoryOwner, MemorySubject, MemoryRecord 等)
- `backend/src/memory/memory_record.cpp` - DTO validateBasic 实现
- `backend/include/pathfinder/memory/memory_store.h` - 内存记忆仓库
- `backend/src/memory/memory_store.cpp` - MemoryStore 实现
- `backend/include/pathfinder/memory/memory_writer.h` - 候选工厂、ID工厂、写入服务
- `backend/src/memory/memory_writer.cpp` - MemoryWriteService 实现
- `backend/include/pathfinder/memory/memory_decay.h` - 记忆衰减服务
- `backend/src/memory/memory_decay.cpp` - MemoryDecayService 实现
- `backend/include/pathfinder/memory/memory_events.h` - 事件与状态变更构造器
- `backend/src/memory/memory_events.cpp` - MemoryEventBuilder / MemoryStateChangeBuilder 实现

### 测试

- 单元测试: `memory_types`, `memory_record`, `memory_store`, `memory_writer`, `memory_decay`
- 集成测试: `p16_memory_from_cognition`, `p16_memory_decay_flow`, `p16_memory_boundary_security`

### 边界

- 不实现 MemorySummary / MemoryBundle / MemoryCompressionPolicy (P17)
- 不实现 KnowledgeClaim / KnowledgeRepository (P18)
- 不实现传播系统 (P20)
- 不依赖 AgentRuntime / Policy / RulePipeline 执行 / GameState 原始读取 / SaveManager / HTTP / JSON
- 隐藏真值扫描: edible_profile / hunger_delta / health_delta / effect_kind / GameState / AgentTickRecord / reward_value / done = / is_done / SaveGame / SaveManager / raw_state / hidden_truth

### 测试结果 (2026-05-17)

- 509/509 全量测试通过
- P16 定向测试: 12/12 通过
- P3-P16 相关回归: 170/170 通过
- 边界扫描全部通过


### P16 审核修复 (2026-05-17)

审核结论：暂不通过 -> 修复后通过

修复内容：

**BLOCKER-1: 工厂入口校验**
- `fromCognitionEvidence()` 首行校验 `evidence.validateBasic()`
- 工厂返回前校验 `candidate.validateBasic()`
- 非法 evidence 返回 `Result::fail`

**BLOCKER-2: 显式enum映射**
- 新增 `mapCognitionSubjectKindToMemoryOwnerKind()` / `mapCognitionTargetKindToMemorySubjectKind()`
- 移除裸 `static_cast`
- Unknown/TestOnly 返回 `Result::fail`

**BLOCKER-3: 证据追踪防线**
- `MemoryRecord::validateBasic()` 要求 `evidence_refs` 非空
- `MemoryEvidenceRef::validateBasic()` 要求 cognition 来源必须有 `cognition_evidence_id` 或 `source_event_id`
- 所有测试构造器补齐合法 evidence ref

**BLOCKER-4: 隐藏真值case-insensitive**
- `containsMemoryForbiddenKey()` 改为先 lowercase 再匹配
- 安全测试新增大小写变体：`gamestate` / `GAMESTATE` / `Health_Delta` / `HIDDEN_TRUTH`

**BLOCKER-5: High/Critical风险统一保护**
- 工厂层：`High/Critical` risk 显式设置 `protect_from_decay`
- Writer强化分支：遇到 `Critical` 候选强制保护已有记忆并设 `lifecycle=Protected`
- 新增 high-risk reinforcement 保护测试

**BLOCKER-6: Discovery/TeachingRelated/Contradiction语义补齐**
- `fromCognitionUpdate()` Created -> 加入 `Discovery`
- `fromCognitionUpdate()` Teachable -> 加入 `TeachingRelated`
- `isSimilarMemory()` contradiction 匹配不依赖其他 kind 交集
- `shouldPromoteShortToMid()` 加入 `Contradiction` / `TeachingRelated`

**ISSUE-1/2/3: 校验补强**
- `MemoryEventDraft::validateBasic()` 校验 owner/subject/scope/lifecycle
- `MemoryStateChangeDraft::validateBasic()` 校验 before/after record
- `MemoryWriteOptions` 校验 `short_to_mid <= mid_to_long`
- `MemoryDecayOptions` 校验 `forgotten <= fading`
- `MemoryIdFactory::makeMemoryId()` 改为 `Result<MemoryId>`

修复后测试：
- 509/509 全量测试通过
- P16 定向测试: 12/12 通过
- P3-P16 相关回归: 189/189 通过
- 边界扫描全部通过

---

## P17: 记忆压缩与检索 (Memory Compression & Recall)

### 新增文件

```
backend/include/pathfinder/memory/memory_summary.h
backend/include/pathfinder/memory/memory_compression.h
backend/include/pathfinder/memory/memory_recall.h
backend/src/memory/memory_summary.cpp
backend/src/memory/memory_compression.cpp
backend/src/memory/memory_recall.cpp
backend/tests/unit/memory/memory_summary_test.cpp
backend/tests/unit/memory/memory_compression_test.cpp
backend/tests/unit/memory/memory_recall_test.cpp
backend/tests/integration/p17/memory_compression_flow_test.cpp
backend/tests/integration/p17/memory_recall_flow_test.cpp
backend/tests/integration/p17/memory_boundary_security_test.cpp
```

### 修改文件

```
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

### 核心类型

**枚举:**
- `MemorySummaryKind`: Unknown/OwnerSubjectPattern/RiskPattern/TeachingPattern/ContradictionPattern/LongTermPattern/TestOnly
- `MemoryCompressionLevel`: Unknown/None/LightSummary/StatisticalSummary/ArchivedProjection/TestOnly
- `MemoryCompressionDecision`: Unknown/Skipped/CreatedSummary/UpdatedSummary/Rejected
- `MemoryRecallMode`: Unknown/ExactSubject/OwnerRecent/ByMemoryKind/ImportantOrCritical/TeachingCandidates/RiskRelated/ContradictionRelated/LongTermOnly/TestOnly
- `MemoryRecallSort`: Unknown/StrengthDesc/LastTouchedDesc/CreatedDesc/ImportanceDesc/EvidenceCountDesc/DeterministicKeyAsc
- `MemoryRecallItemKind`: Unknown/Record/Summary/TestOnly

**DTO:**
- `MemorySummaryKey`, `MemorySummary`, `MemoryCompressionOptions`, `MemoryCompressionPlan`, `MemoryCompressionResult`
- `MemoryRecallQuery`, `MemoryRecallItem`, `MemoryRecallResult`

**服务:**
- `MemorySummaryIdFactory`: 确定性生成 MemorySummaryId
- `MemorySummaryStore`: 内存级摘要存储
- `MemoryCompressionPlanner`: 按 owner+subject 过滤并生成压缩计划
- `MemoryCompressionService`: 生成 MemorySummary 及事件/状态变更草稿
- `MemoryRecallService`: 支持 8 种召回模式的确定性检索

### 设计约束

- 不删除原始 MemoryRecord
- Protected/LongTerm 记忆可参与摘要但不被删除
- 召回排序确定性（ID tie-break）
- 不创建 KnowledgeClaim / KnowledgeRepository
- 不依赖 AgentRuntime / Policy / RulePipeline / GameState / SaveManager / HTTP / JSON
- 复用 P16 hidden truth 黑名单扫描

### 测试结果 (2026-05-17)

- 517/517 全量测试通过 (P16 509 + P17 8)
- P17 定向测试: 8/8 通过
- P3-P17 相关回归: 197/197 通过
- 边界扫描全部通过
