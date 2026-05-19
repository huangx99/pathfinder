# P41 智能 Agent 与世界效果通用执行闭环再次审核报告

## 1. 再次审核结论

结论：再次审核通过。

本次重点复查最新提交：

- `39feb08 refactor: remove P41 driver content bridges`
- `60f5cc0 feat: complete P41 long-term effect DTOs`
- `019b60f feat: implement P41 generic effect execution loop`

和上一轮复查相比，最大的变化是：`goal_execution_system.cpp` 中原来最危险的 P40/P41 内容桥接已经明显收敛。`ChopWoodDriver`、`SharpenToolDriver`、`CounterThreatDriver` 不再直接写死 `cut_wood`、`restore_sharpness`、`repel_beast` 这些 effect_key，而是从 `DriverExecutionState.effect_key` / `AgentPlanStep.effect_key` 传入。

因此，本次结论更新为：

- P41 核心执行底座：通过。
- P41 长期 DTO 与 dry-run 扩展基础：通过。
- P41 Agent 计划到 Driver 再到 WorldEffectExecutor 的链路：通过。
- P41 目标执行层内容 key 硬编码风险：已显著降低。
- H5 / Story 原型层仍有内容 key 与演示默认知识：保留为 P2 风险，不阻塞 P41。

## 2. 已执行验证

### 2.1 编译验证

已执行：

```bash
cmake --build build/backend --target pathfinder_tests_agent_reasoning pathfinder_tests_h5_playable pathfinder_tests_world_interaction pathfinder_tests_goal_execution -j 2
```

结果：通过。

### 2.2 相关测试验证

已执行：

```bash
ctest --test-dir build/backend -R 'p41|agent_reasoning|world_interaction|h5_playable_(p38|execution_status|buttons|story|frontend|danger)|goal_execution' --output-on-failure
```

结果：57/57 通过。

覆盖范围包括：

- P41 DTO / Registry / WorldEffectExecutor。
- 长期效果 dry-run：魔法、机械危机、族群能力、外交协议、瘟疫延迟传播。
- Agent 推理链：饥饿、寒冷、造房、钝斧、驱兽、制作火把、循环检测、搜索限制。
- GoalExecution：目标栈、Driver 注册、阻塞转子目标、狼打断、恢复执行、材料需求、反应产物解析、能力过滤、上下文序列化。
- H5：危险、执行状态、按钮可执行、P38 世界/Agent、失败反馈、剧情首日。

## 3. 本次重点修复确认

### 3.1 Driver 不再直接写死当前原型 effect_key

文件：`backend/src/goal_execution/goal_execution_system.cpp`

已确认：

- `ChopWoodDriver` 不再直接输出 `cut_wood`。
- `SharpenToolDriver` 不再直接输出 `restore_sharpness`。
- `CounterThreatDriver` 不再直接输出 `repel_beast`。
- Driver 发出的 `effect_key` 优先来自 `DriverExecutionState.effect_key`。
- `DriverExecutionState.effect_key` 由 `AgentPlanStep.effect_key` 写入。

评价：通过。

这说明 Driver 更像“行为执行器”，不再自己知道“砍木头等于 cut_wood、驱兽等于 repel_beast”。这是 P41 目标执行层从写死逻辑走向配置化的关键一步。

### 3.2 Driver 前置条件改为读取计划前置条件

文件：`backend/src/goal_execution/goal_execution_system.cpp`

已确认：

- `DriverExecutionState` 新增 `preconditions`。
- Driver 使用 `checkConfiguredPreconditions(...)` 检查 `object.quantity` 和 `object.state`。
- 缺对象时生成 `MissingObject` blocker。
- 工具状态不足时生成 `ToolStateInsufficient` blocker。

评价：通过。

这使“缺火把、缺木头、斧头不锋利”等阻塞不再只依赖某个专用 Driver 的内置判断，而是可以来自计划/反应/效果配置。

### 3.3 Reaction 规则增加执行 key 映射

文件：

- `backend/include/pathfinder/reaction/reaction_rule.h`
- `backend/src/reaction/reaction_rule.cpp`
- `backend/src/reaction/reaction_fixtures.cpp`

已确认：

- `ReactionObjectPattern` 新增 `execution_object_key`。
- `ReactionOutputTemplate` 新增 `execution_output_key`。
- `validateBasic()` 已校验这些 key。
- 核心反应 fixture 给火源、木头、斧头、磨石、产物等补充了执行 key。

评价：通过。

这修复了上一轮报告里最担心的 `productKey(def_xxx) -> runtime_key` 本地映射问题。现在映射至少已经从 GoalExecution 本体挪到了 Reaction 配置/fixture 中，架构方向正确。

### 3.4 ReactionMaterialResolver 不再依赖本地 productKey 表

文件：`backend/src/goal_execution/goal_execution_system.cpp`

已确认：

- `ReactionMaterialResolver::findProducers(...)` 使用 `output.execution_output_key` 判断产物。
- 输入对象使用 `reactionPatternExecutionKey(...)`，优先读取 `pattern.execution_object_key`。
- 旧的 `productKey(...)`、`patternObjectKey(...)` 已从当前文件消失。

评价：通过。

这说明“缺材料 -> 查反应规则 -> 找生产路径”的链条已经比上一版更可扩展。后续新增物品时，理论上应该在内容配置 / Reaction 规则里补 key，而不是改 GoalExecution 核心代码。

## 4. 仍然存在的风险

### 4.1 H5 / Story 层仍然有原型内容 key

文件示例：

- `backend/src/story/story_system.cpp`
- `backend/src/h5_dialog/dialog_scenario.cpp`
- `backend/src/h5_dialog/dialog_turn_service.cpp`
- `backend/src/h5_playable/playable_turn_service.cpp`

仍可看到：

- `cut_wood`
- `restore_sharpness`
- `make_torch`
- `repel_beast`
- `def_torch`
- `def_fire_source`

评价：P2 风险，不阻塞本次 P41。

原因：这些主要属于当前 H5 原型内容、剧情材料、输入演示和投影层。它们不是 P41 世界效果执行器或 GoalExecution Driver 的核心结算分支。但如果未来要做到“加一个新物品完全不改代码”，这些层也必须配置化。

### 4.2 默认同伴火把知识仍是演示辅助逻辑

文件：`backend/src/h5_dialog/dialog_session.cpp`

仍存在：

- `defaultCompanionTorchKnowledge(...)`
- 默认给同伴补一条火把驱兽知识。

评价：P2 风险，不阻塞 P41，但需要在后续玩法验收时重新决定是否保留。

原因：这不是核心推理框架问题，而是 H5 演示为了验证同伴行为所加的 playtest helper。长期版本中，同伴应该通过玩家传授、族群知识、NPC 出身知识或自身观察学习获得知识，而不是由会话默认补齐。

建议后续：

- 把默认知识移到“新手剧本配置”。
- 或提供开关：`playtest_default_companion_knowledge`。
- 正式生存开局默认关闭。

### 4.3 DriverKind 选择仍有少量 action_key 分支

文件：`backend/src/goal_execution/goal_execution_system.cpp`

仍存在：

- `step.action_key == "eat"`
- `step.action_key == "move"`
- `step.action_key == "chop"`

评价：P2 风险。

这不是 effect_key 结果写死，但仍是“动作类型 -> DriverKind”的本地路由。当前可接受，因为 DriverKind 本身就是执行器类型选择。长期更理想的方式是：计划步骤直接携带推荐 `driver_kind`，或者由配置表声明 action_key 到 driver kind 的映射。

## 5. 架构判断

### 5.1 现在是不是还在硬编码结果

结论：核心执行链已经不是硬编码结果。

现在更准确的分层判断是：

- `WorldEffectExecutor`：通过效果配置解释成 `WorldChange`，核心结算通用化。
- `AgentReasoner`：从知识和语义选择候选行动，并把 effect_key 放入计划步骤。
- `GoalExecution`：根据计划步骤和前置条件执行 Driver，不再直接写死当前原型 effect_key。
- `ReactionMaterialResolver`：从 Reaction 规则读取产物和输入执行 key，不再维护本地 def/runtime 映射表。
- `H5 / Story`：仍有原型内容 key，用于当前可玩演示与中文呈现。

所以当前不能说“完全没有任何硬编码”，但可以说：P41 关键架构层已经从硬编码结果转为知识/计划/配置驱动。

### 5.2 这次改动是否提高后续扩展性

结论：明显提高。

新增一个类似“毒蘑菇 / 铁斧 / 魔法火把 / 机械组件”的内容时，后续目标应该变成：

- 添加对象配置。
- 添加知识/效果语义。
- 添加 EffectExecutionSpec。
- 添加 ReactionRule 的 `execution_object_key` / `execution_output_key`。
- 必要时添加 H5 展示文案。

而不是：

- 修改 GoalExecution 里的 `productKey`。
- 修改 Driver 输出固定 effect_key。
- 修改 WorldInteraction 大分支结算。

这是正确方向。

## 6. 建议验收边界

本次建议验收 `39feb08` 对 P41 Driver bridge 的修复。

但后续阶段仍建议明确拆两个任务：

### 6.1 内容目录化阶段

目标：把 H5 / Story / Scenario 里的原型内容 key 从代码迁移到配置或内容包。

验收标准：

- 新增一种普通物品和一种可制作物，不改 H5/Story C++ 主逻辑。
- 中文名、按钮、知识说明、效果说明从配置读取。
- Story 只读内容目录，不自己维护硬编码材料表。

### 6.2 默认知识与开局剧本阶段

目标：把同伴默认火把知识改为剧本配置或测试开关。

验收标准：

- 正式开局：同伴没有凭空知识。
- 教学开局：可配置给同伴预置知识。
- Playtest 开局：可打开辅助知识，方便调试 Agent 行为。

## 7. 最终结论

P41 再次审核通过。

这次提交确实修掉了上一轮复查中最重要的目标执行层风险：Driver 不再直接产出当前原型 effect_key，反应产物与输入对象 key 也开始由 Reaction 规则承载。当前架构已经可以继续往下一阶段推进。

剩余问题主要在 H5 / Story / Scenario 这些演示和表现层，不应阻塞 P41，但必须在后续“内容目录化”阶段处理，否则项目内容扩大后仍会出现新增内容要改多处代码的问题。
