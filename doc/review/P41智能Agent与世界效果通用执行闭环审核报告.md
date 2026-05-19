# P41 智能 Agent 与世界效果通用执行闭环审核报告

## 1. 审核结论

结论：有条件通过，不建议直接视为 P41 完整验收。

当前实现已经完成 P41 第一层核心目标：

- 新增通用效果执行 DTO、枚举、Registry、Executor。
- 当前 H5 原始生存闭环已经接入 `WorldEffectExecutor`。
- `WorldInteractionService` 不再为 `cut_wood`、`make_torch`、`repel_beast` 等写大段专用结算分支。
- Agent 单步执行已经通过 `AgentExecutionCoordinator -> AgentPlanStepExecutor -> WorldEffectExecutor`。
- 同伴没有默认火把时不会凭空驱兽。
- P41 与相邻 Agent / World / H5 测试通过。

但当前实现距离升级版 P41 设计中的“长期世界效果底座”还有明显缺口。

因此本次审核定义为：

- P41 当前最小落地层：通过。
- P41 长期扩展 DTO 层：未完整实现。
- P41 硬编码清理：核心执行层改善明显，但仍有残留。
- P41 完整验收：需要继续修复或明确拆到 P41 后续子阶段。

## 2. 已验证通过的内容

### 2.1 编译通过

已执行：

```bash
cmake --build build/backend --target pathfinder_tests_agent_reasoning pathfinder_tests_h5_playable pathfinder_tests_world_interaction -j 2
```

结果：通过。

### 2.2 针对性测试通过

已执行：

```bash
ctest --test-dir build/backend -R 'p41|agent_reasoning|world_interaction|h5_playable_(p38|execution_status|buttons|story|frontend|companion|danger)' --output-on-failure
```

结果：32/32 通过。

覆盖内容包括：

- P41 执行枚举和 DTO。
- P41 执行 Registry。
- P41 make_torch 原子执行。
- P41 Agent step 无火把阻塞。
- Agent 推理链路。
- WorldInteraction 基础结算。
- H5 P38/P40 相邻功能。

## 3. 关键实现审查

### 3.1 新增 P41 执行模块

文件：`backend/include/pathfinder/agent_reasoning/effect_execution.h`

已实现：

- `EffectExecutionOpKind`
- `EffectExecutionTargetKind`
- `ExecutionValueSourceKind`
- `ExecutionFailureKind`
- `AgentStepExecutionMode`
- `EffectExecutionOperation`
- `EffectExecutionSpec`
- `WorldExecutionRequest`
- `WorldExecutionResult`
- `AgentStepExecutionRequest`
- `AgentStepExecutionResult`
- `EffectExecutionSpecRegistry`
- `ExecutionConditionResolver`
- `WorldEffectExecutor`
- `AgentPlanStepExecutor`
- `AgentExecutionCoordinator`

评价：方向正确，P41 第一层核心骨架成立。

### 3.2 通用世界效果执行器

文件：`backend/src/agent_reasoning/effect_execution.cpp`

已实现：

- 根据 `EffectExecutionSpec` 查找执行规格。
- 通过 P27 evaluator 校验 condition_refs。
- 根据 operation 生成 WorldChange。
- 支持 dry_run。
- 失败时不生成 changes。
- Agent step 可以转成 `WorldExecutionRequest`。

评价：这是本阶段最大进展，已经把原来的“写死结算”抽象成配置解释式执行。

### 3.3 WorldInteractionService 接入 P41

文件：`backend/src/world_interaction/world_services.cpp`

已实现：

- `WorldInteractionService::resolve` 创建 `WorldExecutionRequest`。
- 调用 `WorldEffectExecutor`。
- 将结果转为 `WorldInteractionResult`。

评价：当前 H5 行为结算已经进入 P41 主路径，是正确改造。

### 3.4 Agent 执行接入 P41

文件：`backend/src/agent_reasoning/agent_reasoner.cpp`

已实现：

- `AgentPlanExecutorAdapter::toAutonomyResult` 改为使用 `AgentExecutionCoordinator`。
- 不再只对 `repel_beast` 直接生成 ThreatResolved。

评价：同伴行动不再凭空特判驱兽，方向正确。

## 4. 主要问题

### 4.1 升级版 P41 长期 DTO 没有落地

设计文档已经升级为“智能 Agent 与世界效果通用执行闭环”，要求支持长期扩展 DTO 和枚举，包括：

- `WorldScopeRef`
- `WorldScopeKind`
- `EffectOperationGroupKind`
- `OperationFailurePolicy`
- `TemporalEffectPolicy`
- `TemporalEffectKind`
- `TargetSelectionSpec`
- `TargetSelectionKind`
- `KnowledgeEffectPayload`
- `RelationshipEffectPayload`
- `WorldRuleEffectPayload`
- `RandomExecutionPolicy`
- `ScheduledEffectRef`

当前代码没有实现这些类型。

影响：

- 高级文明、魔法范围效果、机械危机、瘟疫延迟传播、外交协议、知识污染等长期扩展点目前只是文档设计，没有代码 DTO 约束。
- 后续 AI 研发仍可能继续按当前较窄 DTO 实现，导致再次偏向原始生存场景。

建议：

- 如果本阶段目标是 P41 完整版，需要补这些 DTO、枚举、validate、dry-run/unsupported 语义。
- 如果本阶段目标是 P41 第一层落地，需要把未实现内容明确写入审核报告和后续 P41-2/P42。

### 4.2 EffectExecutionOpKind 仍偏当前 H5 场景

当前枚举只有：

- ConsumeObjectQuantity
- AddObjectQuantity
- SetObjectQuantity
- AddObjectStateNumber
- SetObjectStateNumber
- AddObjectTag
- RemoveObjectTag
- ChangeActorNeed
- ChangeThreatLevel
- ResolveThreat
- EmitWorldEvent
- QueueFollowupGoal

缺少升级设计中的：

- ApplyStatusEffect
- RemoveStatusEffect
- ChangeRelationship
- CreateAgreement
- BreakAgreement
- ChangeReputation
- UnlockCapability
- ChangePopulation
- ChangeSystemPressure
- ChangeWorldRule
- PropagateKnowledge
- CorruptKnowledge
- ChangeKnowledgeTrust
- ScheduleDelayedEffect
- StartPeriodicEffect
- StopPeriodicEffect

影响：

- P41 当前代码还不能作为长期世界效果底座的类型基础。
- 未来魔法、瘟疫、机械危机、外交、知识污染仍会缺通用操作入口。

### 4.3 EffectExecutionTargetKind 仍偏小场景

当前枚举只有：

- ActorSelf
- ActorTarget
- ObjectSelf
- ObjectTarget
- Location
- ThreatTarget
- GroupOrTribe
- RuntimeKey

缺少升级设计中的：

- InventoryScope
- Region
- WorldSystem
- Faction
- Civilization
- Megastructure
- KnowledgeGraph
- Agreement
- Timeline

影响：

- 当前类型系统仍默认“对象/地点/威胁”为主。
- 文明级、区域级、知识图谱级、协议级 effect 没有类型承载。

### 4.4 Built-in spec 仍是 C++ 内置表

文件：`backend/src/agent_reasoning/effect_execution.cpp`

`builtInEffectExecutionSpecs()` 目前把当前效果规格写在 C++ 中。

这比旧的 `WorldInteractionService if effect_key` 好很多，因为执行器已经通用化。

但它仍不是最终配置化。

影响：

- 新增普通 effect 仍需要改 C++，除非后续加配置 loader。
- P41 设计要求“允许内置 bootstrap，但必须可替换”，当前还只有内置 bootstrap，没有外部配置入口。

建议：

- 当前可接受为第一层实现。
- 后续必须接入配置包或 fixture loader，并让测试证明新增 effect 不改核心执行器。

### 4.5 ReactionPlanningAdapter 仍存在物品映射硬编码

文件：`backend/src/agent_reasoning/reaction_planning_adapter.cpp`

仍存在：

- `def_fire_source -> camp_fire`
- `def_raw_wood -> wood`
- `def_whetstone -> whetstone`
- `def_axe -> axe`
- tag `combustible/branch_like -> wood_processed`
- condition `source_state:eq:sharp -> axe.sharpness`

影响：

- P41 虽然增加了 `ExecutionConditionResolver`，但 reaction adapter 仍没有完全配置化。
- 新反应体系仍可能继续靠代码猜测物品和前置条件。

建议：

- ReactionRule 应直接声明 execution_effect_key 与 execution precondition mapping。
- Adapter 不应靠 definition_id/tag 猜业务对象。

### 4.6 AgentReasoner 中仍有 effect 特定评分和解释硬编码

文件：`backend/src/agent_reasoning/agent_reasoner.cpp`

仍存在：

- `poison` 被直接判断为 Dangerous。
- `build_house` 用于 durability bonus。
- `ignite_fire`、`provide_warmth`、`build_house` 参与紧急寒冷特殊评分。
- 部分 safe_explanation 根据具体 effect 输出。

影响：

- 虽然执行层已通用化，但决策评分层仍有具体 effect 偏置。
- 后续新增“生火以外的快速取暖方式”或“房子以外的庇护方式”时，仍可能需要改代码。

建议：

- 把这些评分偏置迁移到 EffectSemantics 或 GoalPolicy 配置。
- `poison` 风险应只由 risk_score / semantic_kind 决定，不应写死 effect_key。

### 4.7 GoalExecutionSystem 仍有 effect 到 ActionDriver 的硬编码

文件：`backend/src/goal_execution/goal_execution_system.cpp`

仍存在：

- `restore_sharpness -> SharpenTool`
- `cut_wood -> ChopWood`
- `repel_beast / fire_is_dangerous -> CounterThreat`
- `restore_hunger -> Eat`

影响：

- P40 执行状态展示/驱动仍依赖具体 effect。
- 新 effect 可能无法映射到正确 driver。

建议：

- ActionDriverKind 应来自 effect semantics / execution spec 中的 driver hint。
- 目标执行系统不应知道具体 effect_key。

### 4.8 H5 文案和投影仍有 effect 文案硬编码

文件：

- `backend/src/h5_dialog/dialog_presenter.cpp`
- `backend/src/h5_playable/playable_projection_mapper.cpp`

仍存在大量：

- `effect_key == restore_hunger`
- `effect_key == poison`
- `effect_key == cut_wood`
- `effect_key == make_torch`
- `effect_key == repel_beast`

影响：

- 这属于表现层硬编码，优先级低于执行层。
- 但如果新增大量物品，中文文案仍要改代码。

建议：

- 文案应优先来自 EffectSemanticsDefinition / EffectExecutionSpec.safe_summary_zh_cn。
- H5 mapper 只做 fallback，不做主文案权威。

### 4.9 H5 文本输入解析仍是中文词匹配

文件：`backend/src/h5_dialog/dialog_intent.cpp`

仍存在：

- 文本里有“火把”映射到 torch。
- “火种/点燃/火源/生火”映射到 fire_seed。
- “制作火把”推断目标 fire_seed。

影响：

- 这是 H5 Demo 输入层问题，不影响规则权威。
- 但长期应改成按钮提交结构化 action_id / object_key / target_key。

### 4.10 升级版 P41 要求的未来场景 dry-run 测试缺失

设计要求新增：

- 魔法火球范围副作用 dry-run。
- 机械复制资源链前置条件 dry-run。
- 族群能力解锁 dry-run。
- 外交协议 DTO 测试。
- 瘟疫延迟传播 DTO 测试。

当前测试只覆盖：

- P41 当前 H5 effect。
- make_torch。
- no torch block。
- registry/dto 基础。

影响：

- 当前实现没有证明架构不被原始时代锁死。
- 长期扩展点目前仍停留在文档层。

## 5. 风险评估

### 5.1 当前可接受风险

以下风险可以接受为 P41 第一层：

- Built-in spec 暂时写在 C++。
- H5 文案仍有 fallback 硬编码。
- 中文输入解析仍是 Demo 层。

原因：

- 当前最小原型需要稳定可玩。
- 执行层已明显改善。

### 5.2 当前不可忽略风险

以下风险不能长期保留：

- Long-term DTO 未落地。
- ReactionPlanningAdapter 继续硬编码物品映射。
- AgentReasoner 评分继续硬编码 effect_key。
- GoalExecutionSystem 继续硬编码 effect 到 driver。

原因：

- 这些会影响高级文明、魔法、机械危机等长期扩展。
- 如果不处理，后续 AI 会继续复制旧模式。

## 6. 建议整改优先级

### P0：补齐 P41 升级版 DTO / 枚举 / validate

必须补：

- WorldScopeRef / WorldScopeKind。
- EffectOperationGroupKind。
- OperationFailurePolicy。
- TemporalEffectPolicy / TemporalEffectKind。
- TargetSelectionSpec / TargetSelectionKind。
- RandomExecutionPolicy。
- ScheduledEffectRef。
- unsupported_features。

这一项不一定要完整执行，但必须让类型系统承认长期世界观。

### P0：增加未来场景 dry-run 测试

至少增加：

- 魔法火球范围副作用 dry-run。
- 机械复制周期/系统压力 dry-run。
- 族群能力解锁 dry-run。

测试目标不是实现玩法，而是证明 DTO 不锁死。

### P1：移除 ReactionPlanningAdapter 的业务猜测

把 definition/tag 到 object key 的映射迁到配置或 reaction rule 显式字段。

### P1：把 AgentReasoner 评分偏置迁到配置

让 effect 的紧急性、长期性、风险、耐久价值来自 semantics / goal policy。

### P1：GoalExecutionSystem driver hint 配置化

ActionDriverKind 应由 effect metadata 提供。

### P2：H5 文案配置化

H5 mapper 和 presenter 的 effect 文案应改读 semantics/spec。

### P2：H5 结构化输入替代中文推断

前端按钮提交结构化 action，不依赖中文文本解析。

## 7. 验收建议

本次如果按“P41 第一层核心执行重构”验收：可以通过。

本次如果按“升级后的 P41 长期底座完整设计”验收：不通过，需要补齐 DTO、dry-run 测试和硬编码清理。

建议验收口径：

1. 先标记当前提交为 P41-A：通用执行闭环最小落地。
2. 立刻追加 P41-B：长期扩展 DTO 与 dry-run 验证。
3. 再做 P41-C：硬编码残留清理。

这样既保留当前成果，又不牺牲长期架构。

## 8. 审核结论

审核结论：有条件通过。

当前代码完成了 P41 最重要的第一步：把 H5 原始生存闭环和 Agent 单步执行接入通用世界效果执行器。

但它还没有完全达到升级版 P41 作为长期底座的要求。

必须继续补：

- 多尺度世界 scope。
- 延迟/持续/周期 effect。
- 副作用/风险 effect 分组。
- 可复盘随机预留。
- 知识污染、关系、协议、能力解锁等 DTO。
- 未来场景 dry-run 测试。
- 旧硬编码残留清理。

否则 P41 会停留在“当前 H5 原始生存闭环通用化”，而不是“高级文明、魔法世界、机械危机都能复用的长期底座”。
