# P41 智能 Agent 与世界效果通用执行闭环复查报告

## 1. 复查结论

结论：复查通过，可以进入下一阶段，但仍保留 P1/P2 残留风险。

追加复查结论（资源归属修正）：通过。

本次追加复查确认：P41 不再把“世界里存在火把”直接等价为“同伴可以使用火把”。玩家制作出的火把默认记录到玩家的 actor 数量视图，同伴即使已经学会“火把能驱赶野兽”，如果没有被显式分配或授权，也只能返回“知道但没有可用火把”，不能凭空取用玩家物品。

本次复查针对提交：

- `60f5cc0 feat: complete P41 long-term effect DTOs`
- `019b60f feat: implement P41 generic effect execution loop`

和上一版审核相比，P41 的核心问题已经明显改善：

- 长期世界效果 DTO 与枚举已经补入代码，不再只是文档设计。
- 魔法、机械危机、族群能力、外交协议、瘟疫延迟传播等未来场景已有 dry-run / DTO 校验测试。
- P41 主执行链已经形成：`Agent 计划 -> AgentStepExecutor -> WorldEffectExecutor -> WorldChange`。
- 当前 H5 原型的制作、驱兽、工具损耗、Agent 阻塞与重规划路径仍能通过测试。

因此本次不再维持“有条件通过，不建议完整验收”的旧结论，更新为：

- P41 核心执行底座：通过。
- P41 长期 DTO 基础：通过。
- P41 当前 H5 可玩闭环兼容：通过。
- P41 配置目录化与表现层去硬编码：未完全完成，作为后续风险保留。

## 2. 已执行验证

### 2.0 追加验证：资源归属与同伴权限

已执行：

```bash
cmake --build build/backend --target pathfinder_tests_h5_playable pathfinder_tests_world_interaction pathfinder_tests_agent_reasoning pathfinder_tests_goal_execution -j2
ctest --test-dir build/backend -R 'h5_playable_|goal_execution_|agent_reasoning_|world_interaction_' --output-on-failure
```

结果：58/58 通过。

重点覆盖：

- `h5_playable_companion_taught_torch_autonomy` 已改为验证“同伴学会火把知识但没有显式火把权限时不能行动”。
- 玩家制作火把后，火把不会自动成为营地共享物。
- 同伴自动行动反馈不再出现“从营地共享物品里取用”。
- Agent 自主执行使用 actor-scoped resource view，不再默认读取全局物品数量。

### 2.1 编译验证

已执行：

```bash
cmake --build build/backend --target pathfinder_tests_agent_reasoning pathfinder_tests_h5_playable pathfinder_tests_world_interaction -j 2
```

结果：通过。

### 2.2 针对性测试验证

已执行：

```bash
ctest --test-dir build/backend -R 'p41|agent_reasoning|world_interaction|h5_playable_(p38|execution_status|buttons|story|frontend|danger)' --output-on-failure
```

结果：37/37 通过。

覆盖的重点测试包括：

- `p41_execution_enums_and_dto`
- `p41_execution_registry`
- `p41_world_effect_executor_make_torch_atomic`
- `p41_agent_step_executor_blocks_without_torch`
- `p41_magic_fireball_area_side_effect_dry_run`
- `p41_mechanical_replication_system_pressure_dry_run`
- `p41_tribe_capability_unlock_dry_run`
- `p41_diplomacy_agreement_dto_validation`
- `p41_plague_delayed_spread_dto_validation`
- `agent_reasoning_build_house_chain`
- `agent_reasoning_dull_axe_chain`
- `agent_reasoning_beast_torch`
- `agent_reasoning_make_torch_reaction_rule`
- `h5_playable_p38_agent`
- `h5_playable_buttons_executable`

## 3. 修复项复查

### 3.1 长期世界效果 DTO 已补齐

文件：`backend/include/pathfinder/agent_reasoning/effect_execution.h`

已确认新增并接入校验的类型包括：

- `WorldScopeKind`
- `EffectOperationGroupKind`
- `OperationFailurePolicy`
- `TemporalEffectKind`
- `TargetSelectionKind`
- `WorldScopeRef`
- `TemporalEffectPolicy`
- `TargetSelectionSpec`
- `KnowledgeEffectPayload`
- `RelationshipEffectPayload`
- `WorldRuleEffectPayload`
- `RandomExecutionPolicy`
- `ScheduledEffectRef`

评价：通过。

这解决了上一版审核中的核心问题：高级文明、魔法范围效果、机械危机、瘟疫、外交、知识污染等不再没有类型落点。

### 3.2 EffectExecutionOpKind 已扩展长期操作

文件：`backend/include/pathfinder/agent_reasoning/effect_execution.h`

已确认新增长期操作：

- `ApplyStatusEffect`
- `RemoveStatusEffect`
- `ChangeRelationship`
- `CreateAgreement`
- `BreakAgreement`
- `ChangeReputation`
- `UnlockCapability`
- `ChangePopulation`
- `ChangeSystemPressure`
- `ChangeWorldRule`
- `PropagateKnowledge`
- `CorruptKnowledge`
- `ChangeKnowledgeTrust`
- `ScheduleDelayedEffect`
- `StartPeriodicEffect`
- `StopPeriodicEffect`

评价：通过。

当前这些长期操作主要作为 DTO / dry-run / validation 基础存在，尚未全部接入真实世界状态写入。这是合理的阶段边界，因为真实结算需要后续世界系统、阵营系统、时间线系统、知识图谱系统继续承接。

### 3.3 EffectExecutionTargetKind 已扩展长期目标

文件：`backend/include/pathfinder/agent_reasoning/effect_execution.h`

已确认新增长期目标：

- `InventoryScope`
- `Region`
- `WorldSystem`
- `Faction`
- `Civilization`
- `Megastructure`
- `KnowledgeGraph`
- `Agreement`
- `Timeline`

评价：通过。

这使 P41 不再只服务当前 H5 小场景，而是可以承接区域、阵营、文明、世界规则、时间线等长期扩展。

### 3.4 未来场景 dry-run 测试已补充

文件：`backend/tests/test_agent_reasoning.cpp`

已确认新增未来场景测试：

- 魔法火球范围与副作用 dry-run。
- 机械复制导致系统压力 dry-run。
- 族群能力解锁 dry-run。
- 外交协议 DTO 校验。
- 瘟疫延迟传播 DTO 校验。

评价：通过。

这些测试不是为了让当前 H5 立刻出现魔法或机械危机，而是为了证明 P41 的抽象不再被红果、火把、野兽这些原型内容锁死。

### 3.5 反应系统前置条件桥接已改善

文件：

- `backend/include/pathfinder/reaction/reaction_rule.h`
- `backend/src/reaction/reaction_rule.cpp`
- `backend/src/reaction/reaction_fixtures.cpp`
- `backend/src/agent_reasoning/reaction_planning_adapter.cpp`

已确认新增：

- `ReactionExecutionPrecondition`
- `ReactionObjectPattern::execution_preconditions`
- `ObjectReactionRule::execution_preconditions`
- 从 Reaction 规则提取 `PlanPrecondition` 的转换逻辑。

评价：通过。

这使“制作火把缺材料，需要先砍木头 / 生火”这类链式规划更接近从反应配置推导，而不是完全靠单独写死。

## 4. 仍然存在的问题

### 4.1 P40 最小桥接仍有内容 ID 映射

文件：`backend/src/goal_execution/goal_execution_system.cpp`

仍存在：

- `productKey("def_torch") -> "torch"`
- `productKey("def_wood_processed") -> "wood_processed"`
- `productKey("def_fire_source") -> "camp_fire"`
- `productKey("def_raw_wood") -> "wood"`
- `productKey("def_axe") -> "axe"`
- `productKey("def_whetstone") -> "whetstone"`
- `patternObjectKey(...)` 对 P28 fixture 输入模式做映射。

代码中已经有 `TODO(P41-config-catalog)` 标记。

评价：非阻塞，但必须保留为后续风险。

原因：这部分属于 P40/P41 之间的最小桥接，用于让已有 H5 原型跑通。它不是核心执行器里的结算大分支，但如果长期不处理，会影响“加一个新物品像配置喝水一样简单”的目标。

建议后续阶段补：

- ObjectDefinitionId 到 runtime object key 的配置目录。
- Reaction pattern 到可执行对象实例的统一解析服务。
- GoalExecution 不再维护本地 fixture 映射表。

### 4.2 部分 ActionDriver 仍带原型动作 key

文件：`backend/src/goal_execution/goal_execution_system.cpp`

仍存在：

- `ChopWoodDriver` 生成 `cut_wood`。
- `SharpenToolDriver` 生成 `restore_sharpness`。
- `CounterThreatDriver` 生成 `repel_beast`。

评价：非阻塞，但属于 P1 架构债。

当前这些 driver 已经不再直接结算世界结果，而是发出 effect/action 请求，由 P41 执行链继续处理。因此它们不是最危险的“写死结果”。但长期看，driver 应从 `AgentPlanStep` / `EffectExecutionSpec` / `ReactionRule` 读取推荐执行 key，而不是内置当前原型内容。

建议后续阶段补：

- DriverCommand 的 `effect_key` 来源改为计划步骤或配置绑定。
- Driver 只负责行为流程，不负责知道“砍木头对应 cut_wood”。
- 原型 driver 保留为 fallback，但必须可被配置覆盖。

### 4.3 H5 输入与展示层仍有中文词和 effect label 映射

文件示例：

- `backend/src/h5_dialog/dialog_intent.cpp`
- `backend/src/h5_dialog/dialog_presenter.cpp`
- `backend/src/h5_playable/playable_projection_mapper.cpp`
- `backend/src/h5_dialog/dialog_session.cpp`

仍存在：

- 中文输入词到对象 key 的映射。
- effect_key 到中文展示文案的映射。
- 部分 H5 投影对 `repel_beast` 等 key 做展示判断。

评价：非阻塞，属于表现层 / Demo 层风险。

这不会否定 P41 核心执行链，但会影响后续内容扩展体验。长期目标应该是从对象配置、知识配置、效果配置中生成展示文案，而不是 H5 层自己认识所有物品。

## 5. 架构判断

### 5.1 目前是否还是写死结果

结论：核心结果不再是旧式写死大分支，但还没有达到完全配置目录化。

现在的状态可以分成三层：

- 核心执行层：已转为 `EffectExecutionSpec -> Operation -> WorldChange`，方向正确。
- 目标执行桥接层：仍有 P40 最小桥接映射，保留 TODO。
- H5 表现层：仍有 demo 文案和输入词映射。

所以准确说法是：

- 不是“为了完成任务而把结果写死在 UI 或 WorldInteraction 里”。
- 也还不是“所有物品、动作、目标、文案都能纯配置添加”。
- 当前处于“核心执行器通用化已完成，外围内容目录化还需要继续补”的阶段。

### 5.2 当前能否支撑后续 P42/P43

结论：可以。

原因：

- 长期 DTO 已有类型落点。
- dry-run 可验证未来场景不会污染当前世界状态。
- 不可真实执行的长期 op 在非 dry-run 下会 fail-safe，而不是悄悄成功。
- Agent 单步执行已经通过 P41 执行器，不再绕过核心结算。

后续阶段可以在不推翻 P41 的情况下继续接入：

- 配置目录系统。
- 真实时间线调度。
- 阵营 / 族群 / 文明关系状态。
- 知识传播、污染、信任变化的真实写入。
- 地图与区域作用域。
- 高级 NPC 的持续目标执行。

## 6. 验收建议

本次建议验收 P41，但不要关闭以下后续任务：

### P1：内容目录化与 Driver 去内容 key

目标：消除 `goal_execution_system.cpp` 中的 P40 bridge，使新增物品、反应、目标不需要改 C++ 映射表。

验收标准：

- 新增一个类似“毒蘑菇 / 普通蘑菇 / 铁斧 / 魔法火把”的内容，不需要改 GoalExecution 本体。
- DriverCommand 的 effect_key 能从计划或配置传入。
- Reaction pattern 能通过配置解析到 runtime object key。

### P1：长期 op 真实结算分批落地

目标：让已经定义的长期操作逐步接入真实世界状态。

优先顺序建议：

1. `ApplyStatusEffect` / `RemoveStatusEffect`
2. `ScheduleDelayedEffect`
3. `ChangeRelationship` / `ChangeReputation`
4. `PropagateKnowledge` / `ChangeKnowledgeTrust`
5. `ChangeWorldRule` / `ChangeSystemPressure`

### P2：H5 表现层配置化

目标：让界面展示从配置读取中文名、说明、知识文案、按钮动作，而不是 H5 层维护 key 表。

验收标准：

- 添加新物品时，H5 自动显示中文名、可尝试动作、知识描述。
- H5 不再需要为每个 effect_key 手写 label。

## 7. 最终结论

P41 复查通过。

这次修复已经把上一轮最关键的问题补上：长期 DTO、长期操作枚举、长期目标枚举、未来场景测试都已经进入代码和测试体系。当前系统已经可以作为“智能 Agent 与世界效果通用执行闭环”的基础层继续发展。

但必须明确：P41 不是项目终点。它完成的是通用执行底座，不是完整内容生产系统。下一步最重要的是把 P40/P41 桥接层中的内容 key 映射逐步收敛到配置目录，否则未来内容越来越多时，仍会回到“加东西要改代码”的风险。

## 8. P40 接入同伴自主行动补充复查

复查日期：2026-05-20

### 8.1 问题结论

本次复查确认：P40 目标执行系统并不是没有实现，真正问题是“同伴自主行动路径”之前没有稳定走完 P39 推理计划到 P40 Driver 再到 P41 世界结算的闭环。

具体表现是：

- 同伴知道 `repel_beast` 时，如果自己没有火把，不能凭空使用玩家的火把，这是正确的。
- 但同伴同时掌握 `make_torch / ignite_fire / cut_wood / restore_sharpness` 等前置知识时，自主行动应该继续走准备链，而不是停在“缺少火把”。
- P40 对 `wood_processed` 的材料解释和 P41 / P39 的“wood.processed 派生材料”解释存在口径差异，导致执行层误判缺材料。

### 8.2 本次修正

已完成以下修正：

- `AgentAutonomyService` 的同伴自主行动路径接入 `GoalExecutionSystem::tick`。
- 同伴行动现在使用 actor-scoped snapshot，只能看见和使用自己可支配的资源。
- P40 `quantityOf` 和 `MaterialRequirementEvaluator` 支持把 `wood.processed` 解释为可用的 `wood_processed` 派生材料。
- P40 Driver 产出的 `DriverCommand` 统一交给 `WorldInteractionService`，再由 P41 `WorldEffectExecutor` 结算，不在 H5 或 autonomy 层直接伪造结果。
- 新增底层回归 `world_interaction_companion_p40_torch`，验证“同伴无火把但掌握完整链路时，会通过 P40 制作自己的火把”。

### 8.3 不是写死结果的判断

本次修复不是针对“火把”写一条绕过逻辑。

实际链路是：

1. 同伴拥有传播来的知识 Claim。
2. P39 根据威胁目标生成计划。
3. P40 根据计划步骤检查材料和前置条件。
4. P40 生成 DriverCommand。
5. WorldInteractionService 解析命令。
6. P41 WorldEffectExecutor 依据 effect spec 产生 WorldChange。
7. WorldChangeApplier 把生成物归属到行动 actor。

当前仍有少量 effect 文案摘要用于 H5 展示，但规则结果不由这些文案决定。

### 8.4 自检结果

已执行相关回归：

```text
ctest --test-dir build/backend -R 'h5_playable_|world_interaction_|goal_execution_|agent_reasoning_' --output-on-failure
```

结果：58/58 通过。

覆盖重点：

- H5 可玩流程没有被破坏。
- 世界交互与威胁逻辑通过。
- P39 推理链仍能选择火把、造火把、斧头维护等计划。
- P40 材料、阻塞、Driver、投影相关测试通过。
- 新增同伴 P40 自主造火把回归通过。

### 8.5 后续风险

仍建议后续继续处理：

- `wood_processed` 这种派生材料现在已有统一解释，但长期应进入内容目录 / 材料别名配置，不应无限扩散 C++ 特例。
- autonomy summary 仍有少量中文文案 key 映射，后续内容目录化时应迁移到配置。
- 当前同伴自主行动是“一次触发执行一步”，后续大世界需要把执行上下文持久化，让 Agent 可跨多回合继续未完成目标。
