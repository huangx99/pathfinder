# P51 Agent 网格目标执行任务卡设计

## 1. 阶段定位

P51 是 V2.0 开放世界网格架构中的“Agent 把知识转化为行动”的阶段。

它接在以下阶段之后：

```text
P39：知识链式推理与目标规划。
P40：动态目标执行、内部阻塞、外界打断。
P41：智能 NPC 通用执行闭环重构。
P43：统一 WorldCommand 管线。
P44：二维世界网格和 actor 坐标。
P45：背包、物品实例和归属接口。
P46：世界生成器和资源分布。
P47：采集、砍伐、挖掘命令。
P48：制作与世界反应。
P49：世界经验进入认知 / 记忆 / 知识。
P50：教学与无知 NPC 基础行动门禁。
```

P51 的一句话定位：

```text
让一个已经拥有知识的 NPC / Agent，能在世界网格中根据目标推理下一步，并通过 WorldCommand 管线真正行动。
```

P50 解决“NPC 能不能被教会”，P51 解决“NPC 被教会之后能不能自己做”。

P51 不是重新设计 AI，不是另起一个行为树，也不是把旧 H5 同伴逻辑搬回来。P51 是把已有的 P39/P40/P41 推理与执行能力，接入 V2 的世界网格、背包归属、资源节点、制作反应和 Command 管线。

## 2. 为什么现在做

P50 完成后，系统已经具备：

```text
玩家拥有自己的 KnowledgeClaim。
玩家可以通过 Teach command 把知识传播给 NPC。
NPC 拥有自己的 KnowledgeOwner 和 KnowledgeClaim。
NpcBasicActionKnowledgeGate 可以判断 NPC 是否知道某个单步行动。
```

但如果没有 P51，会出现新的断层：

```text
NPC 知道“红果可以吃”，但不会在饿的时候自己找红果。
NPC 知道“木头可以采集”，但不会移动到资源点并执行采集命令。
NPC 知道“制作火把需要材料”，但不会因为缺材料而先去采集。
NPC 遇到外界打断时，无法暂停原目标并处理更紧急问题。
NPC 的行动如果不走 WorldCommand，就会绕过 P47/P48/P49，导致不会消耗材料、不会形成经验、不会进入学习闭环。
```

所以 P51 必须在 P52 野兽生态之前完成。否则狼、蜘蛛、野生 NPC 等 Agent 只能写死行动，无法复用知识、目标、Command 和学习闭环。

P51 也必须在 P53 H5 V2 之前完成。否则前端即使有地图，也只能展示玩家命令，不能展示 NPC 自主行动的权威结果。

## 3. 阶段目标

P51 完成后，系统必须具备以下能力：

```text
1. 系统可以对一个 actor_key 执行 Agent tick。
2. Agent tick 能读取 actor 坐标、可见世界、背包、知识、已有执行上下文。
3. Agent 的行动资格必须经过 P50 NpcBasicActionKnowledgeGate。
4. Agent 的计划必须来自 P39/P40/P41 的目标/推理/执行结构，不能写死火把、红果、木头、狼。
5. Agent 执行动作必须编译成 WorldCommandDto，并走 WorldCommandPipeline。
6. Agent command source 必须是 WorldCommandSource::AgentDecision 或 BeastDecision。
7. Agent 不能直接修改 WorldRuntime、InventoryRuntime 或 KnowledgeRepository。
8. Command 执行结果必须回写 AgentExecutionContext，用于下一 tick 继续、暂停、重试或完成。
9. Command 产生的 experiences 必须保留给 P49 世界学习链路，不能被 Agent 层吞掉。
10. 内部阻塞能变成子目标，例如缺材料、缺工具、距离不够、知识不足。
11. 外界打断能暂停当前目标并插入高优先级目标，例如威胁出现、天气变化、路径被堵。
12. 输出安全 projection / events / trace，方便前端和调试查看“NPC 为什么这么做”。
```

## 4. 本阶段不解决的问题

P51 不解决以下问题：

```text
不做完整 A* 寻路，只做相邻移动或最小下一步移动接口；复杂寻路后续单独扩展。
不做野兽生态行为细节，P52 做狼、蜘蛛、野生 Agent 感知、攻击、绕行和学习。
不做 H5 V2 地图 UI，P53 做。
不做大规模族群排程，只做单 actor 的目标执行上下文；多人协作后续扩展。
不做新的知识形成规则，仍由 P49/P18-P21 负责。
不做新的物品反应规则，仍由 P48/P28/P42 content 负责。
不做前端可修改 NPC 目标的自由编辑器，只允许后端安全 command / system tick 输入。
不做完整存档恢复，P54 做。
```

P51 可以保留 fake port / fake pipeline 做测试，但真实实现边界必须围绕 V2 WorldCommand。

## 5. 核心设计原则

### 5.1 Agent 行动必须走 Command

Agent 不能直接调用采集、制作、移动、吃、使用等底层 service。

正确流程：

```text
AgentExecutionCoordinator
-> AgentPlanCommandCompiler
-> WorldCommandDto
-> WorldCommandPipeline.execute
-> WorldCommandExecutionResult
-> AgentExecutionContext update
```

禁止：

```text
if goal == gather then runtime.addItem(...)
if target == torch then inventory.consume(...)
if wolf_near then threat = 0
```

原因：只有 Command 管线能统一产生事件、状态变更、projection patch、经验记录、学习闭环和复盘 trace。

### 5.2 知识是行动资格，不是装饰文本

P51 不允许 NPC 因为代码知道某个 action 就执行。

每个会影响世界的行动，在编译或提交前必须做知识门禁：

```text
actor_key
subject_object_key
action_key
effect_key
target_object_key
allow_hypothesis
allow_risk_action
```

门禁来源：

```text
NpcBasicActionKnowledgeGate
KnowledgeRepository
KnowledgeOwner(actor_key)
```

如果 NPC 没有知识，结果必须是：

```text
BlockedKnowledgeMissing
不生成世界修改 command
可输出“需要学习/被教学/观察”的安全原因
```

### 5.3 推理只决定意图，执行必须验证现实

P39/P40 可以提出计划，但 P51 必须在真实 V2 runtime 中再次验证：

```text
actor 是否存在。
目标坐标是否可达或相邻。
物品是否真的在 actor 背包或地图上。
资源节点是否还有 charges。
工具是否属于 actor / 营地授权库存。
制作材料是否足够。
```

推理层认为“可以做”，不等于执行层一定成功。

失败时不能强行完成目标，只能生成 blocker 或 replan 信号。

### 5.4 内部阻塞转子目标

内部阻塞是 Agent 自己计划链里缺的东西。

典型内部阻塞：

```text
MissingObject：缺红果、木头、火把。
InsufficientQuantity：木头数量不足。
ToolStateInsufficient：斧头钝了。
LocationMismatch：离资源点太远。
KnowledgeMissing：不知道怎么做。
ResourceDepleted：资源点枯竭。
PathBlocked：路被堵。
```

P51 必须把可处理 blocker 转成子目标：

```text
缺材料 -> AcquireObject。
距离不够 -> MoveTo。
工具状态不足 -> RepairOrImproveTool。
知识不足 -> SeekTeaching / Observe。
资源枯竭 -> FindAlternativeResource。
```

### 5.5 外界打断独立于内部阻塞

外界打断不是 Agent 自己缺材料，而是世界突然变化。

典型外界打断：

```text
ThreatAppeared：狼进入视野。
ThreatEscalated：狼靠近或攻击。
WeatherChanged：突然降温、下雨。
DependentInDanger：幼崽/同伴受威胁。
ResourceDestroyed：目标资源被别人拿走。
PathBlocked：落石堵路。
CommandOverride：玩家直接命令。
```

外界打断必须进入 InterruptSystem，而不是塞进普通 blocker。

结果可以是：

```text
Ignore。
ObserveOnly。
PauseAndInsertEmergencyGoal。
CancelCurrentGoal。
ReplanCurrentGoal。
ResumeAfterHandling。
FailCurrentGoal。
```

### 5.6 归属和材料不能绕过 P45

P51 所有材料判断必须通过 P45 背包/归属接口。

禁止：

```text
直接遍历 runtime.entities 然后认为 actor 拥有该物品。
直接把地图上的物品当成营地共享库存。
制作产物直接写入数组。
```

正确方式：

```text
查询 InventoryOwnerRef(actor/camp/container)。
通过 ItemLocationRef 判断 OnMap / InInventory / Equipped。
通过 WorldCommand 执行 Pickup / Drop / Craft / Gather。
```

### 5.7 Projection 只解释，不裁决

P51 输出给前端的是：

```text
Agent 当前目标。
当前计划步骤。
为什么选择该步骤。
被什么阻塞。
发出了哪个 command。
command 结果。
下一步建议。
```

前端不能根据这些字段自己改变世界或判断成功。

### 5.8 后续可扩展到所有 Agent 等级

P51 的结构必须支持：

```text
InstinctAgent：狼、蜘蛛、小动物，本能行动少量目标。
SimpleAgent：普通 NPC，会吃、采集、躲避。
CognitiveAgent：人类同伴，会被教学、会链式准备材料。
AdvancedAgent：高级文明、机械体、魔法生物，会长链计划和系统级目标。
```

不能写死“companion”。

## 6. 玩家体验映射

P51 对玩家体验的直接提升：

```text
玩家不只是教一个静态百科，而是在教会一个会行动的生命。
NPC 不再凭空会做事，也不再永远傻站着。
NPC 的行动能被解释：他为什么去采集、为什么先移动、为什么停下、为什么需要你教。
玩家能看见知识传播带来的文明扩散：一个 NPC 学会采集后，真的能去采集。
```

最小体验闭环：

```text
玩家探索 -> 学到“某资源可以采集” -> 教 NPC -> NPC 获得知识 -> 系统 tick -> NPC 选择采集目标 -> NPC 移动到相邻格 -> NPC 发出 Gather/Chop command -> 世界变化 -> NPC 形成自己的经验或巩固知识。
```

## 7. 头脑风暴：P51 必须覆盖的未来场景

### 7.1 生存场景

```text
NPC 饿了，知道红果能缓解饥饿。
红果在附近格子，NPC 移动过去采集，再吃。
红果不够，NPC 寻找替代食物。
吃了毒蘑菇中毒后，NPC 不会再把该知识当成正向食物知识。
```

### 7.2 制作场景

```text
NPC 知道火把能驱赶危险。
没有火把，NPC 查找制作火把的知识。
缺木头，插入采集木头子目标。
斧头钝了，插入打磨斧头子目标。
材料足够后，发出 Craft command。
```

### 7.3 建造场景

```text
天气寒冷，族群缺房子。
NPC 知道房子能御寒。
计划建房子。
缺木头 -> 砍树。
缺石片 -> 采集/拾取。
建造过程中狼出现 -> 暂停建造，处理威胁，再恢复。
```

### 7.4 危险场景

```text
野兽靠近。
NPC 知道火把能驱赶。
NPC 检查自己是否有火把。
没有火把但有制作知识时，进入准备链。
威胁太近，来不及制作时，选择逃跑或呼救。
```

### 7.5 野生 Agent 场景

```text
狼不是脚本事件，而是 InstinctAgent。
狼有自己的目标：饥饿、恐惧、领地。
狼观察到火后形成危险经验。
狼后续会绕行或保持距离。
```

P51 不实现狼生态细节，但目标执行框架必须允许 BeastDecision source。

### 7.6 高级文明场景

```text
高级文明 Agent 有长链目标，例如建设能源塔。
计划会拆成采矿、运输、制造、组装、防御。
每一步仍然通过 command，不直接改世界。
```

### 7.7 魔法世界场景

```text
法师知道区域魔法能驱散毒雾。
缺法力时先休息或寻找法力源。
区域魔法通过 CastAreaEffect command 触发。
```

### 7.8 机械危机场景

```text
机械体需要能源。
知道充能站可以恢复能源。
路径上遇到敌对单位，插入防御或绕行目标。
```

### 7.9 社会协作场景

```text
一个 NPC 缺知识，另一个 NPC 是专家。
目标执行可以生成 SeekTeaching 子目标。
后续 P50 Teach command 可以完成 NPC 教 NPC。
```

### 7.10 玩家干预场景

```text
玩家命令 NPC 停止当前目标。
CommandOverride 外界打断进入 InterruptSystem。
NPC 暂停、取消或改目标，但不会让前端直接修改执行上下文。
```

## 8. 模块划分

| 模块 | 中文含义 | 职责 | 输入 | 输出 | 禁止事项 |
|---|---|---|---|---|---|
| `WorldAgentExecutionTypes` | Agent 执行类型 | 枚举、DTO、validateBasic、toString/fromString | actor、goal、plan、command、result | 安全 DTO | 不写业务规则 |
| `WorldAgentExecutionContextStore` | 执行上下文存储 | 保存 actor 当前目标栈、计划、阻塞、暂停上下文 | actor_key、context | context snapshot | 不保存隐藏真相到前端 |
| `WorldAgentContextBuilder` | 世界上下文构建器 | 聚合 runtime、inventory、knowledge、visible resources | ports | `WorldAgentDecisionContext` | 不直接改 runtime |
| `WorldAgentGoalSelector` | 目标选择器 | 从需求、外界打断、queued goal 选择当前目标 | decision context | goal candidate | 不写死具体物品 |
| `WorldAgentReasoningBridge` | 推理桥 | 调用 P39/P40/P41 reasoner / goal execution 结构 | goal + context | plan / blockers | 不重写推理器 |
| `AgentActionKnowledgeGuard` | 行动知识门禁桥 | 调用 P50 gate 判断是否可执行 | planned step | gate result | 不读取玩家知识 |
| `AgentPlanCommandCompiler` | 计划转命令 | 把 plan step 编译为 WorldCommandDto | step + context | command 或 blocker | 不直接执行 service |
| `AgentCommandSubmitter` | Command 提交口 | 把 AgentDecision command 送进 pipeline | command | execution result | 不绕过 pipeline |
| `AgentExecutionCoordinator` | 单 Agent tick 协调器 | 串起上下文、推理、门禁、编译、提交、更新 | tick request | tick result | 不写内容特判 |
| `WorldAgentProjectionBridge` | 投影桥 | 输出 Agent 行动解释和 trace | tick result | events / deltas / patch | 不裁决规则 |
| `ExternalInterruptCollector` | 外界打断收集 | 从事件、威胁、区域效果中生成 interrupt | runtime/events | interrupt list | 不和内部 blocker 混用 |
| `WorldAgentArchitectureGuardTests` | 架构扫描 | 防止 H5、内容写死、直接改数组 | source scan | pass/fail | 不做假测试 |

## 9. 核心枚举

### 9.1 `WorldAgentExecutionDecision`

中文：Agent 执行决策结果。

枚举值：

```text
Unknown：未知，默认值，不允许业务正常返回。
NoopNoGoal：没有可执行目标，本 tick 不动作。
SelectedGoal：选中了目标但尚未发 command。
PlannedStep：生成了计划步骤。
IssuedCommand：已向 WorldCommandPipeline 提交 command。
CommandSucceeded：command 成功。
CommandBlocked：command 被规则阻断。
CommandFailed：command 失败。
PausedForInterrupt：因外界打断暂停。
InsertedSubGoal：因内部阻塞插入子目标。
WaitingForKnowledge：缺少知识，等待教学/观察。
CompletedGoal：目标完成。
TestOnly：测试专用。
```

规则：

```text
Unknown 可作为默认值。
TestOnly 只能在测试中使用。
toString/fromString 必须稳定，小写 snake_case 或与项目现有风格保持一致。
```

### 9.2 `WorldAgentExecutionFailureKind`

中文：Agent 执行失败原因。

枚举值：

```text
None：无失败。
InvalidRequest：请求无效。
ActorMissing：actor 不存在。
ContextBuildFailed：上下文构建失败。
KnowledgeMissing：缺少行动知识。
ReasoningFailed：推理失败。
PlanEmpty：没有可执行计划。
CommandCompileFailed：计划无法编译为 command。
CommandPipelineFailed：command 管线执行失败。
CommandBlocked：command 被规则阻断。
RuntimeUnavailable：runtime port 不可用。
InventoryUnavailable：inventory port 不可用。
OwnershipInvalid：归属不合法。
Interrupted：被外界打断。
MaxDepthExceeded：目标栈过深。
ForbiddenBySafety：安全门禁拒绝。
TestOnly：测试专用。
```

### 9.3 `WorldAgentGoalSourceKind`

中文：目标来源。

枚举值：

```text
Unknown：未知。
InternalNeed：内部需求，例如饥饿、寒冷、疲劳。
PlayerOrder：玩家命令。
WorldThreat：世界威胁。
ExternalInterrupt：外界打断。
KnowledgeOpportunity：知识推导出的机会。
QueuedGoal：已有队列目标。
SystemMaintenance：系统维护目标，例如恢复工具、补材料。
TestOnly：测试专用。
```

### 9.4 `WorldAgentStepCommandKind`

中文：计划步骤编译出的命令类别。

枚举值：

```text
Unknown：未知。
Move：移动。
Gather：采集。
Chop：砍伐。
Mine：挖掘矿物。
Dig：挖掘地面。
Pickup：拾取。
Drop：丢弃。
Eat：食用。
Use：使用。
Craft：制作。
Teach：教学。
Attack：攻击。
Flee：逃离。
Wait：等待。
CastAreaEffect：释放区域效果。
TriggerAreaEvent：触发区域事件。
TestOnly：测试专用。
```

该枚举不能替代 `WorldCommandKind`，只是 Agent 层编译诊断用。

## 10. 核心 DTO

### 10.1 `WorldAgentTickRequest`

```cpp
struct WorldAgentTickRequest {
    std::string request_id;
    std::string actor_key;
    uint64_t tick = 0;
    bool allow_issue_command = true;
    bool allow_replan = true;
    bool allow_hypothesis_knowledge = true;
    bool allow_risk_action = false;
    uint32_t max_subgoal_depth = 8;
    std::vector<std::string> queued_goal_ids;
    std::vector<ExternalInterruptSignal> external_interrupts;
    std::vector<std::string> reason_keys;
};
```

说明：

```text
actor_key 是本次 tick 的 Agent。
allow_issue_command=false 时只做 dry run，用于调试和测试。
allow_hypothesis_knowledge 控制是否允许假设知识驱动行动。
allow_risk_action 控制危险行动是否允许执行。
```

### 10.2 `WorldAgentDecisionContext`

```cpp
struct WorldAgentDecisionContext {
    std::string context_id;
    std::string actor_key;
    WorldActorRuntime actor;
    WorldRuntimeSnapshot runtime_snapshot;
    InventoryRuntimeSnapshot inventory_snapshot;
    std::vector<KnowledgeClaim> actor_claims;
    std::vector<VisibleResourceRef> visible_resources;
    std::vector<VisibleEntityRef> visible_entities;
    std::optional<ExecutionContext> existing_execution_context;
    std::vector<ExternalInterruptSignal> external_interrupts;
    uint64_t tick = 0;
};
```

规则：

```text
这是后端内部 DTO，不直接暴露隐藏真相给前端。
visible_* 必须是 actor 可见范围内的数据。
actor_claims 只能是 actor 自己 owner 的知识。
```

### 10.3 `WorldAgentCompiledCommand`

```cpp
struct WorldAgentCompiledCommand {
    bool ok = false;
    WorldAgentStepCommandKind step_command_kind;
    WorldCommandDto command;
    std::vector<InternalBlocker> blockers;
    std::vector<std::string> reason_keys;
};
```

规则：

```text
ok=true 时必须有 command。
command.source 必须是 AgentDecision 或 BeastDecision。
command.actor_key 必须等于 Agent actor_key。
command.context.parent_command_id 可指向 system tick 或 agent tick id。
```

### 10.4 `WorldAgentTickResult`

```cpp
struct WorldAgentTickResult {
    bool ok = false;
    WorldAgentExecutionDecision decision;
    WorldAgentExecutionFailureKind failure_kind;
    std::string actor_key;
    std::optional<AgentGoal> selected_goal;
    std::optional<AgentPlan> selected_plan;
    std::optional<AgentPlanStep> selected_step;
    std::optional<WorldCommandDto> issued_command;
    std::optional<WorldCommandExecutionResult> command_result;
    ExecutionContext execution_context_after;
    std::vector<InternalBlocker> blockers;
    std::vector<InterruptDecision> interrupt_decisions;
    std::vector<WorldEventDto> events;
    std::vector<WorldStateDeltaDto> state_deltas;
    std::optional<WorldProjectionPatchDto> projection_patch;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;
};
```

## 11. 核心接口

### 11.1 `IWorldAgentExecutionContextStore`

```cpp
class IWorldAgentExecutionContextStore {
public:
    virtual ~IWorldAgentExecutionContextStore() = default;
    virtual Result<std::optional<ExecutionContext>> findByActor(const std::string& actor_key) const = 0;
    virtual Result<void> put(const ExecutionContext& context) = 0;
    virtual Result<void> clearActor(const std::string& actor_key) = 0;
};
```

### 11.2 `IAgentCommandPipelinePort`

```cpp
class IAgentCommandPipelinePort {
public:
    virtual ~IAgentCommandPipelinePort() = default;
    virtual Result<WorldCommandExecutionResult> executeAgentCommand(const WorldCommandDto& command) = 0;
};
```

说明：

```text
真实实现包装 WorldCommandPipeline。
测试可以提供 fake pipeline。
P51 只通过这个 port 提交 command。
```

### 11.3 `IAgentVisibleWorldQueryPort`

```cpp
class IAgentVisibleWorldQueryPort {
public:
    virtual ~IAgentVisibleWorldQueryPort() = default;
    virtual Result<WorldActorRuntime> findActor(const std::string& actor_key) const = 0;
    virtual Result<WorldRuntimeSnapshot> visibleSnapshotForActor(const std::string& actor_key) const = 0;
    virtual Result<std::vector<VisibleResourceRef>> visibleResourcesForActor(const std::string& actor_key) const = 0;
};
```

说明：

```text
不能返回全图隐藏信息。
调试可以返回 full snapshot，但必须标记 allow_hidden_debug。
```

### 11.4 `IAgentInventoryQueryPort`

```cpp
class IAgentInventoryQueryPort {
public:
    virtual ~IAgentInventoryQueryPort() = default;
    virtual Result<InventoryRuntimeSnapshot> snapshotForActorDecision(const std::string& actor_key) const = 0;
};
```

### 11.5 `IAgentKnowledgeQueryPort`

```cpp
class IAgentKnowledgeQueryPort {
public:
    virtual ~IAgentKnowledgeQueryPort() = default;
    virtual Result<std::vector<KnowledgeClaim>> claimsForActor(const std::string& actor_key) const = 0;
};
```

## 12. 核心服务

### 12.1 `WorldAgentContextBuilder`

职责：

```text
读取 actor、可见世界、背包、知识、执行上下文。
构建 WorldAgentDecisionContext。
```

失败情况：

```text
actor 不存在 -> ActorMissing。
runtime / inventory / knowledge query 失败 -> ContextBuildFailed。
```

### 12.2 `WorldAgentGoalSelector`

职责：

```text
先处理外界打断。
再处理已有 active goal。
再处理 queued goal。
最后可从内部需求 / 世界威胁生成目标。
```

P51 MVP 目标来源优先级：

```text
1. ExternalInterrupt。
2. Active execution context。
3. QueuedGoal。
4. InternalNeed / WorldThreat 的最小 fake input。
5. 无目标则 NoopNoGoal。
```

### 12.3 `WorldAgentReasoningBridge`

职责：

```text
把 WorldAgentDecisionContext 转为 P39 ReasoningRequest。
调用 AgentReasoner 或 P40 GoalExecutionSystem。
返回 AgentPlan / blockers。
```

禁止：

```text
自己写“饿了吃红果”。
自己写“缺木头去砍树”。
```

### 12.4 `AgentActionKnowledgeGuard`

职责：

```text
从 AgentPlanStep 中提取 subject/action/effect/target。
调用 NpcBasicActionKnowledgeGate。
根据 allow_hypothesis / allow_risk_action 决定是否允许。
```

如果计划步骤是 Move / Wait，可以允许不需要对象知识；但 Gather / Chop / Eat / Use / Craft / Attack / CastAreaEffect 必须经过门禁或明确的本能能力门禁。

### 12.5 `AgentPlanCommandCompiler`

职责：把计划步骤转成 WorldCommandDto。

映射示例：

```text
MoveTo -> WorldCommandKind::Move + target_coord。
Gather -> WorldCommandKind::Gather + target_entity_id/resource node id。
ChopWood -> WorldCommandKind::Chop。
UseObject -> WorldCommandKind::Use。
Eat -> WorldCommandKind::Eat。
Craft -> WorldCommandKind::Craft + recipe_key。
Teach -> WorldCommandKind::Teach + target_actor_key + knowledge_key。
Wait -> WorldCommandKind::Wait。
```

规则：

```text
所有 command.source = AgentDecision 或 BeastDecision。
所有 command.command_id 必须可追踪：agent_tick_id + step_id + attempt。
不能把执行结果写进 parameters。
parameters 只能放安全、结构化、非隐藏输入。
```

### 12.6 `AgentExecutionCoordinator`

职责：P51 主入口。

流程：

```text
validate request。
build context。
collect/evaluate interrupts。
select goal。
reason / reuse plan。
select next step。
knowledge guard。
compile command。
if dry-run：返回 projection，不提交。
submit command via pipeline。
merge command result events/deltas/patch。
update execution context。
store context。
return WorldAgentTickResult。
```

### 12.7 `WorldAgentProjectionBridge`

职责：输出可读、可调试、安全的 Agent 行动投影。

输出字段建议：

```text
actor_key。
current_goal_summary。
current_step_summary。
decision。
failure_kind。
issued_command_kind。
blocker summaries。
interrupt summaries。
trace keys。
```

禁止输出：

```text
隐藏真相。
未观察到的地图资源。
规则内部私密数值。
```

## 13. 关键流程

### 13.1 NPC 学会采集后自动行动

```text
玩家 Gather -> P49 形成知识。
玩家 Teach -> P50 传播给 NPC。
SystemTick -> P51 tick NPC。
ContextBuilder 读取 NPC 知识。
ReasoningBridge 生成 AcquireObject / Gather 计划。
KnowledgeGuard 通过。
CommandCompiler 生成 Gather command。
Pipeline 执行 P47。
P47 输出资源变化和 experiences。
P49 后续学习链路可处理 experiences。
ExecutionContext 记录当前步骤完成。
```

### 13.2 缺材料触发子目标

```text
目标：制作某物。
ReasoningBridge 生成 Craft step。
Compiler / precondition check 发现材料不足。
InternalBlockerResolver 生成 AcquireObject 子目标。
下一 tick 执行采集 / 拾取。
材料满足后恢复 Craft step。
```

### 13.3 距离不够触发移动

```text
目标资源可见但不相邻。
Compiler 不直接发 Gather。
生成 Move command，目标为下一步邻格。
移动成功后下一 tick 再尝试 Gather。
```

### 13.4 外界打断暂停当前目标

```text
当前目标：砍树。
外界事件：ThreatAppeared。
InterruptSystem 决定 PauseAndInsertEmergencyGoal。
当前 GoalFrame paused。
插入 ReduceThreat goal。
威胁处理完成后 ResumeAfterHandling。
恢复砍树。
```

## 14. 与已有系统关系

| 系统 | P51 如何使用 | 禁止 |
|---|---|---|
| P39 AgentReasoner | 生成目标计划 | 不重写推理规则 |
| P40 GoalExecutionSystem | 处理目标栈、阻塞、打断 | 不把外界打断写成普通 if |
| P41 通用执行闭环 | 保持效果/前置/执行可配置 | 不新增内容硬编码 |
| P43 WorldCommandPipeline | 提交所有 Agent 行动 | 不直接调用底层 service |
| P44 WorldRuntime | 读取 actor 坐标、可见世界 | 不直接改 runtime map |
| P45 Inventory | 查询/验证物品归属 | 不假设营地物品自动共享 |
| P47 Harvest | 通过 command 执行采集 | 不手写资源减少 |
| P48 Reaction/Craft | 通过 command 执行制作 | 不手写产物生成 |
| P49 Learning | 保留 command experiences | 不吞掉经验 |
| P50 Teaching/Gate | 判断 NPC 知识是否能行动 | 不读玩家知识 |

## 15. 实现建议目录

```text
backend/include/pathfinder/world_agent_execution/
backend/src/world_agent_execution/
backend/tests/unit/world_agent_execution/
backend/tests/integration/world_agent_execution/
```

CMake target：

```text
pathfinder_world_agent_execution
pathfinder_tests_world_agent_execution
pathfinder_tests_world_agent_execution_integration
```

## 16. 必须实现清单

```text
1. WorldAgentExecutionTypes：枚举、DTO、validateBasic、toString/fromString。
2. InMemoryWorldAgentExecutionContextStore：测试和 MVP 可用。
3. WorldAgentContextBuilder：通过 port 构建决策上下文。
4. WorldAgentGoalSelector：选择 active / queued / interrupt / no goal。
5. WorldAgentReasoningBridge：适配 P39/P40，不重写推理。
6. AgentActionKnowledgeGuard：调用 P50 gate。
7. AgentPlanCommandCompiler：把 step 编译成 WorldCommandDto。
8. AgentExecutionCoordinator：主 tick 编排。
9. WorldAgentProjectionBridge：安全投影与 trace。
10. Fake ports + 集成测试。
11. 架构扫描测试。
```

## 17. 验收标准

P51 通过必须满足：

```text
1. NPC 没有知识时，不能执行采集/制作/使用等行动。
2. NPC 被教学后，能通过门禁并发出 AgentDecision command。
3. Agent command 必须走 pipeline，fake pipeline 测试要证明没有直接改状态。
4. 距离不够时先 Move，不直接远程采集。
5. 缺材料时返回 blocker / subgoal，不凭空生成材料。
6. 外界打断能暂停当前目标并插入紧急目标。
7. Command result 的 events / deltas / experiences 不丢失。
8. 扫描确认无 H5 依赖、无内容写死、无直接 runtime map 修改。
9. P43-P50 相关回归测试通过。
```

## 18. 非阻塞后续扩展

```text
完整 A* 寻路。
多 NPC 协作和资源预约冲突解决增强。
长期职业系统：采集者、工匠、猎人、教师。
族群级目标：建村、迁徙、防御、贸易。
野兽生态：P52。
H5 V2 地图：P53。
存档复盘：P54。
强化学习训练样本：读取 Agent tick trace + command result。
```

## 19. 实现阶段注意事项

```text
不要为了让测试过而写死 red_berry / torch / wood / wolf / companion。
不要绕过 P50 gate。
不要绕过 WorldCommandPipeline。
不要直接写 runtime.entities / actors / resource_nodes / inventory arrays。
不要让前端或测试传入“NPC 已学会”的 claim json。
不要把 H5 demo 的 playable_turn_service 逻辑搬进 P51。
不要在 P51 内新增内容规则；内容来自 JSON / content registry / existing knowledge。
```

## 20. 自检结论

本设计符合 `doc/00_设计文档编写要求.md`：

```text
说明了阶段定位、为什么现在做、目标、不做范围、设计原则。
定义了模块、枚举、DTO、接口、核心流程、验收标准。
明确了权威边界：知识归 P50/P18-P21，世界变化归 WorldCommand，物品归属归 P45，学习归 P49。
面向低上下文 AI，避免实现者靠聊天记录猜。
```
