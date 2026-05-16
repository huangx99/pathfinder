# 30_P5Agent最小定义与观察任务卡设计

## 1. 设计目标

P5 的目标是在 P0-P4 已经完成的基础设施之上，建立 Agent 的最小工程闭环：

```text
定义一个 Agent 是谁。
让 Agent 只能看到允许看到的观察。
让 Agent 拥有可枚举、可校验的动作空间。
让 Agent 把意图转换成 CommandEnvelope。
让同一套 RulePipeline 继续作为唯一结算入口。
```

一句话：

```text
P5 不做聪明 AI，P5 先把“智能体如何安全地产生命令”固定下来。
```

P5 结束后，研发应能证明：

```text
蜘蛛、狼、人类、族群、高级文明都可以被同一套 AgentDefinition 表达。
Agent 不能直接修改 GameState。
Agent 不能直接调用 Resolver。
Agent 不能读取真实隐藏状态，只能读取 AgentObservation。
AgentIntent 可以被 AgentCommandAdapter 转成合法 CommandEnvelope。
P3 的 unknown_fruit + eat 闭环可以从 AgentIntent 发起，再走 RulePipeline 执行。
```

## 2. 为什么 P5 做 Agent 最小闭环

目前 P0-P4 已经完成：

```text
P0：基础类型、StrongId、Result、Error、随机、时间、版本。
P1：配置与命令，CommandEnvelope / ActionCommand / ActionTarget。
P2：状态、事件、管线，GameState / EventStream / RulePipeline。
P3：未知对象第一条规则闭环，unknown fruit + eat。
P4：复盘与协议前置，CommandReplayLog / RandomReplayLog / ReplayRunner / ProtocolEnvelope。
```

但系统还缺一个关键抽象：

```text
谁来根据观察选择命令？
```

如果 P5 直接实现完整 AI，会出现问题：

```text
AI 容易绕过 CommandEnvelope。
AI 容易直接读 GameState 真相。
AI 容易把预测结果当成规则结果。
蜘蛛、狼、人类、族群、文明会各写一套行为逻辑。
后续强化学习无法复用稳定 Observation / Action / Reward / Done 接口。
```

所以 P5 只做最小闭环：

```text
AgentDefinition -> AgentBinding -> AgentObservation -> AgentActionSpace -> AgentIntent -> CommandEnvelope
```

不做完整调度、不做复杂策略、不做战斗 AI。

## 3. 本阶段解决什么问题

P5 解决五个问题：

```text
1. Agent 类型契约：不同尺度智能体如何用统一定义表达。
2. Agent 绑定契约：Agent 控制哪个 Actor、Group、Tribe 或 Civilization。
3. Agent 观察契约：Agent 能看到什么，不能看到什么。
4. Agent 动作契约：当前能选择哪些动作，每个动作需要什么目标。
5. Agent 命令适配：AgentIntent 如何转换为 CommandEnvelope。
```

P5 必须支持的测试级场景：

```text
先驱者 Agent：观察 unknown_fruit，产生 eat / experiment 意图，转成 CommandEnvelope，进入 RulePipeline。
蜘蛛 Micro Agent：观察 fire danger，产生 flee / avoid risk 意图，证明小型生物可表达。
狼 SmallCreature Agent：观察 pack signal，产生 call_pack / group intent，证明群体协作意图可表达。
高级文明 Agent：只做 definition validation，证明顶级 Agent 可扩展，不实现内容。
```

注意：

```text
蜘蛛和狼在 P5 只需要证明“可定义、可观察、可产生意图”。
如果现有规则没有 flee / call_pack resolver，P5 不应强行实现新玩法 resolver。
```

## 4. 本阶段不解决的问题

P5 严禁提前实现：

```text
完整 Agent 调度器。
复杂 AI 策略算法。
行为树。
GOAP。
效用 AI。
神经网络。
强化学习训练算法。
路径寻路。
战斗结算。
族群分裂结算。
不同族群战争结算。
高级文明内容系统。
H5 前端。
HTTP 服务。
WebSocket 服务。
多人同步。
新存档格式。
数据库。
```

P5 允许：

```text
新增 agent 模块。
新增 agent context pack。
新增 Agent 相关基础枚举。
新增 AgentDefinition / AgentBinding / AgentObservation / AgentActionSpace / AgentIntent / AgentCommandAdapter。
新增测试级 Spider / Wolf / Alien definition fixture。
新增 P5 集成测试：AgentIntent eat -> CommandEnvelope -> RulePipeline。
```

P5 禁止：

```text
Agent 直接修改 GameState。
Agent 直接创建 StateChangeDraft。
Agent 直接调用 EatObjectResolver。
Agent 从 GameState 读取 DebugOnly / Hidden 真相。
Agent 自己应用 StateChangeSet。
Agent 自己生成最终 EventRecord。
Agent 在复盘时重新思考替代历史决策。
Agent 测试为了通过而修改 P3 规则。
P5 顺手实现 use / combine / teach / fight 真实玩法规则。
```

## 5. P5 与 P0-P4 的关系

P5 可以依赖：

```text
foundation：StrongId、Result、ErrorDetail、Tick、StateVersion、DecisionId、TargetId。
command：CommandEnvelope、ActionCommand、ActionTarget、CommandSource、CommandIntent。
state：GameState 只允许被观察构建器读取，不允许 Agent 直接持有可变引用。
event：EventRecord / EventStream 可作为观察摘要来源。
pipeline：RulePipeline 只允许集成测试提交 CommandEnvelope，不允许 AgentCommandAdapter 调用。
rules：UnknownFruitFixture 只允许 P5 集成测试复用。
cognition：CognitionState / KnowledgeClaim 可作为观察摘要来源，Agent 不能绕过投影读取真实对象效果。
replay：P5 不强制集成，但 AgentDecisionId / command correlation 要为 P6 训练轨迹预留。
protocol：P5 不强制输出协议，但 Agent 决策结果要能被未来 ProtocolEnvelope 包装。
```

P5 不应依赖：

```text
frontend。
HTTP / WebSocket。
完整存档。
数据库。
复杂配置热加载。
完整战争系统。
完整族群系统实现。
```

关键关系：

```text
Agent 负责选择。
Command 负责表达意图。
Pipeline 负责结算。
StateChangeGate / MinimalStateApplier 负责状态变更。
Event 负责事实记录。
Cognition 负责主观认知。
Replay 负责未来重放。
Protocol 负责未来对外输出。
```

## 6. P5 最小闭环定义

### 6.1 输入

```text
AgentDefinition：player_pioneer。
AgentBinding：agent_player_001 -> actor_001。
GameState：UnknownFruitFixture::createInitialState()。
AgentObservation：actor_001 看见 obj_unknown_fruit_001 的可交互摘要。
AgentActionSpace：包含 experiment/eat 候选动作。
AgentIntent：actor_001 对 obj_unknown_fruit_001 执行 experiment/eat。
```

### 6.2 适配

```text
AgentCommandAdapter 接收 AgentIntent。
校验 agent_id / actor_id / target / intent_type。
创建 CommandEnvelope。
CommandEnvelope.source = CommandSource::Ai 或 Player。
CommandEnvelope.payload.intent = CommandIntent::Experiment。
CommandEnvelope.payload.actor_id = actor_001。
CommandEnvelope.payload.targets[0] = obj_unknown_fruit_001。
CommandEnvelope.correlation_id 关联 AgentDecisionId。
```

### 6.3 执行

```text
P5 集成测试把 CommandEnvelope 提交给 RulePipeline。
RulePipeline 继续走 P3 的 eat resolver。
断言 hunger 下降、object consumed、cognition claim 产生。
```

### 6.4 蜘蛛 / 狼测试级闭环

```text
Spider Micro Agent：定义 micro scale + reflex cognition + single body，观察 fire_danger，动作空间生成 avoid_risk/flee 意图。
Wolf SmallCreature Agent：定义 small_creature scale + instinct cognition + pack-capable profile，观察 pack_signal，动作空间生成 call_group 意图。
```

如果当前 CommandIntent 没有精确表达 flee / call_group：

```text
flee 映射为 AvoidRisk 意图，但不提交 RulePipeline。
call_group 映射为 GroupCoordination 类 AgentIntent，但不提交 RulePipeline。
```

P5 的重点不是规则结算这些行为，而是证明 Agent 抽象能稳定表达这些行为。

## 7. 类型与中文翻译

| 英文类型 | 中文名 | P5 是否实现 | 说明 |
|---|---|---:|---|
| AgentId | 智能体 ID | 是 | 在 agent/types.h 中定义本模块 StrongId 别名，标识一个决策体 |
| AgentDefinitionId | 智能体定义 ID | 是 | 在 agent/types.h 中定义本模块 StrongId 别名，标识配置层模板 |
| AgentScale | 智能体尺度 | 是 | 微型、小型、个体、队伍、族群、文明、星际 |
| AgentCognitionBand | 智能体认知层级 | 是 | 反射、本能、联想、工具、社会、抽象、文明 |
| AgentEmbodiment | 具身形式 | 是 | 单体、多体、分布式、远程、系统 |
| AgentControllerType | 控制器类型 | 是 | 玩家、AI、脚本、训练、系统、测试 |
| AgentDefinition | 智能体定义 | 是 | 描述 Agent 类型、尺度、认知、权限、默认策略 |
| AgentProfile | 智能体画像 | 是 | 风险偏好、好奇心、社交性、协作性 |
| AgentBinding | 智能体绑定 | 是 | Agent 与 Actor / Group / Tribe 的控制关系 |
| AgentObservation | 智能体观察 | 是 | 可见、可用、主观的信息输入 |
| AgentObservedObject | 观察到的对象 | 是 | 对象 ID、交互线索、认知摘要，不含真实隐藏效果 |
| AgentObservedThreat | 观察到的威胁 | 是 | 危险来源、危险等级、置信度 |
| AgentActionSpace | 动作空间 | 是 | 当前可选候选动作集合 |
| AgentActionCandidate | 动作候选 | 是 | 一个可选动作及其目标要求 |
| AgentIntent | 智能体意图 | 是 | 决策后但未提交规则层的行动表达 |
| AgentDecision | 智能体决策 | 是，最小版 | observation + action + intent 的记录 |
| AgentCommandAdapter | 意图转命令适配器 | 是 | AgentIntent -> CommandEnvelope |
| AgentPolicy | 智能体策略 | 否 | P5 不实现策略算法，只预留接口概念 |
| AgentRuntime | 智能体运行时 | 否 | P5 严禁实现完整调度器 |
| RewardSignal | 奖励信号 | 否 | 强化学习阶段实现，P5 只预留字段名 |
| DoneSignal | 终止信号 | 否 | 强化学习阶段实现，P5 只预留字段名 |

## 8. 推荐目录

P5 允许新增：

```text
context_packs/agent_p5.md
backend/dev_notes/agent.md
backend/include/pathfinder/agent/README.md
backend/include/pathfinder/agent/types.h
backend/include/pathfinder/agent/definition.h
backend/include/pathfinder/agent/binding.h
backend/include/pathfinder/agent/observation.h
backend/include/pathfinder/agent/action_space.h
backend/include/pathfinder/agent/intent.h
backend/include/pathfinder/agent/command_adapter.h
backend/src/agent/types.cpp
backend/src/agent/definition.cpp
backend/src/agent/binding.cpp
backend/src/agent/observation.cpp
backend/src/agent/action_space.cpp
backend/src/agent/intent.cpp
backend/src/agent/command_adapter.cpp
backend/tests/unit/agent/agent_types_test.cpp
backend/tests/unit/agent/agent_definition_test.cpp
backend/tests/unit/agent/agent_binding_test.cpp
backend/tests/unit/agent/agent_observation_test.cpp
backend/tests/unit/agent/agent_action_space_test.cpp
backend/tests/unit/agent/agent_intent_test.cpp
backend/tests/unit/agent/agent_command_adapter_test.cpp
backend/tests/integration/p5/agent_unknown_fruit_flow_test.cpp
backend/tests/integration/p5/spider_flee_fire_test.cpp
backend/tests/integration/p5/wolf_call_pack_test.cpp
```

P5 不允许新增：

```text
backend/include/pathfinder/agent/runtime.h
backend/src/agent/runtime.cpp
backend/include/pathfinder/agent/policy_tree.h
backend/src/agent/policy_tree.cpp
backend/include/pathfinder/agent/rl_environment.h
backend/src/agent/rl_environment.cpp
frontend/
h5/
http/
websocket/
```

说明：

```text
如果研发确实需要一个接口占位，只能在 README 或 context pack 中描述，不创建可被误用的 runtime / rl_environment 代码文件。
```

## 9. 数据结构设计要求

### 9.1 AgentScale

```cpp
enum class AgentScale {
    Unknown,
    MicroCreature,
    SmallCreature,
    Individual,
    Squad,
    Tribe,
    Civilization,
    Planetary,
    Interstellar
};
```

中文说明：

```text
MicroCreature：微型生物，例如蜘蛛、昆虫。
SmallCreature：小型生物，例如狼、蛇、鸟。
Individual：个体智能体，例如先驱者、人类、外星个体。
Squad：小队智能体，例如狩猎队、侦察队。
Tribe：族群智能体，例如一个部落整体。
Civilization：文明智能体，例如一个高级文明。
Planetary：星球级智能体，后续预留。
Interstellar：星际级智能体，后续预留。
```

### 9.2 AgentCognitionBand

```cpp
enum class AgentCognitionBand {
    Unknown,
    Reflex,
    Instinct,
    Associative,
    ToolUse,
    SocialLearning,
    AbstractReasoning,
    Civilizational
};
```

中文说明：

```text
Reflex：反射，只对刺激做简单反应。
Instinct：本能，可逃跑、觅食、呼叫同类。
Associative：联想，可把对象和效果建立经验关系。
ToolUse：工具使用，可尝试使用物体。
SocialLearning：社会学习，可传播和模仿知识。
AbstractReasoning：抽象推理，可规划、假设、比较风险。
Civilizational：文明认知，可组织长期战略和技术路线。
```

### 9.3 AgentEmbodiment

```cpp
enum class AgentEmbodiment {
    Unknown,
    SingleBody,
    MultiBody,
    DistributedGroup,
    RemoteInfluence,
    SystemProcess
};
```

中文说明：

```text
SingleBody：单体具身，一个 Agent 控制一个 Actor。
MultiBody：多体具身，一个 Agent 控制多个 Actor。
DistributedGroup：分布式群体，一个 Agent 代表族群或组织。
RemoteInfluence：远程影响，不直接具身在本地对象。
SystemProcess：系统进程，用于测试、脚本、训练环境。
```

### 9.4 AgentIntentType

```cpp
enum class AgentIntentType {
    Unknown,
    Eat,
    Use,
    AvoidRisk,
    Flee,
    CallGroup,
    Explore,
    Teach,
    Combine,
    Fight,
    Wait
};
```

P5 映射要求：

```text
Eat -> CommandIntent::Experiment，目标类型 Object，允许提交 RulePipeline。
Use -> CommandIntent::Experiment 或 Exploit，P5 只校验，不提交 RulePipeline。
AvoidRisk -> CommandIntent::AvoidRisk，P5 只校验。
Flee -> CommandIntent::AvoidRisk，P5 只校验。
CallGroup -> CommandIntent::AvoidRisk 或 Fight 不足以表达时，只生成 AgentIntent，不生成 CommandEnvelope。
Explore -> CommandIntent::Experiment，P5 只校验。
Teach -> CommandIntent::Teach，P5 只校验。
Combine -> CommandIntent::Combine，P5 只校验。
Fight -> CommandIntent::Fight，P5 只校验。
Wait -> 不生成 CommandEnvelope，只生成 AgentDecision。
```

注意：

```text
AgentIntentType 是 Agent 层的意图。
CommandIntent 是规则命令层的意图。
二者不能混用。
```

## 10. 核心接口草案

### 10.1 AgentDefinition

```cpp
struct AgentDefinition {
    AgentDefinitionId definition_id;
    std::string display_name_key;
    AgentScale scale = AgentScale::Unknown;
    AgentCognitionBand cognition_band = AgentCognitionBand::Unknown;
    AgentEmbodiment embodiment = AgentEmbodiment::Unknown;
    AgentControllerType default_controller = AgentControllerType::Unknown;
    AgentProfile profile;

    foundation::Result<void> validateBasic() const;
};
```

校验要求：

```text
definition_id 非空且格式合法。
display_name_key 非空。
scale 不能是 Unknown。
cognition_band 不能是 Unknown。
embodiment 不能是 Unknown。
default_controller 不能是 Unknown。
profile 必须通过 validateBasic。
```

### 10.2 AgentProfile

```cpp
struct AgentProfile {
    double curiosity = 0.0;
    double caution = 0.0;
    double aggression = 0.0;
    double sociality = 0.0;
    double cooperation = 0.0;
    double learning_rate_hint = 0.0;

    foundation::Result<void> validateBasic() const;
};
```

校验要求：

```text
所有数值范围必须在 0.0 到 1.0。
P5 不使用这些数值做复杂 AI，只作为后续策略输入。
```

### 10.3 AgentBinding

```cpp
struct AgentBinding {
    AgentId agent_id;
    foundation::EntityId primary_actor_id;
    std::optional<foundation::TribeId> tribe_id;
    std::vector<foundation::EntityId> controlled_actor_ids;
    AgentControlAuthority authority = AgentControlAuthority::Unknown;

    foundation::Result<void> validateBasic() const;
};
```

校验要求：

```text
agent_id 必须合法。
primary_actor_id 必须合法。
authority 不能是 Unknown。
controlled_actor_ids 不允许重复。
如果 controlled_actor_ids 非空，必须包含 primary_actor_id。
P5 不检查 Actor 是否真的存在，存在性属于观察构建或规则执行阶段。
```

### 10.4 AgentObservation

```cpp
struct AgentObservation {
    AgentId agent_id;
    foundation::EntityId observer_actor_id;
    foundation::StateVersion state_version;
    foundation::Tick observed_tick;
    std::vector<AgentObservedObject> objects;
    std::vector<AgentObservedThreat> threats;
    std::vector<AgentObservedKnowledge> knowledge_claims;

    foundation::Result<void> validateBasic() const;
};
```

观察规则：

```text
Observation 是主观投影，不是真实世界全量状态。
Observation 可以包含“我认为可食用置信度 0.6”。
Observation 不允许包含“真实毒性 = true”这种隐藏真相。
Observation 不关注颜色、形状、马赛克揭示等级。
Observation 只关注可食用、可使用、效果、风险、证据、置信度、可传授性。
```

### 10.5 AgentActionSpace

```cpp
struct AgentActionCandidate {
    foundation::ActionId action_id;
    AgentIntentType intent_type = AgentIntentType::Unknown;
    std::vector<command::ActionTargetType> required_target_types;
    bool command_supported = false;
    std::string reason_key;

    foundation::Result<void> validateBasic() const;
};

struct AgentActionSpace {
    AgentId agent_id;
    foundation::StateVersion state_version;
    std::vector<AgentActionCandidate> candidates;

    foundation::Result<void> validateBasic() const;
};
```

校验要求：

```text
ActionSpace 可以为空，但 validateBasic 要通过。
候选 action_id 不能重复。
intent_type 不能是 Unknown。
required_target_types 不能包含 None。
command_supported 表示当前是否能转为 CommandEnvelope。
command_supported = false 的候选仍然可用于测试和未来扩展，但不能提交 RulePipeline。
```

### 10.6 AgentIntent

```cpp
struct AgentIntent {
    AgentId agent_id;
    foundation::DecisionId decision_id;
    foundation::EntityId actor_id;
    AgentIntentType intent_type = AgentIntentType::Unknown;
    std::vector<command::ActionTarget> targets;
    double confidence = 0.0;
    std::string reason_key;

    foundation::Result<void> validateBasic() const;
};
```

校验要求：

```text
agent_id / decision_id / actor_id 必须合法。
intent_type 不能是 Unknown。
targets 对于 Eat / Use / Teach / Combine / Fight 必须非空。
confidence 范围必须是 0.0 到 1.0。
reason_key 必须非空，便于调试和未来复盘。
```

### 10.7 AgentCommandAdapter

```cpp
class AgentCommandAdapter {
public:
    foundation::Result<command::CommandEnvelope> toCommandEnvelope(
        const AgentIntent& intent,
        command::CommandSource source,
        foundation::Tick issued_tick) const;
};
```

适配规则：

```text
Adapter 只做数据转换和基础校验。
Adapter 不执行 RulePipeline。
Adapter 不读取或修改 GameState。
Adapter 不生成 EventRecord。
Adapter 不生成 StateChangeDraft。
Adapter 不调用 Resolver。
Adapter 对不支持的 AgentIntentType 返回 common_unsupported 或 command_invalid_argument。
```

## 11. TASK-P5-000：创建 Agent 上下文包

任务名称：创建 P5 Agent 实现上下文包。

任务目标：

```text
让后续 AI 只读一份小上下文就能实现 P5，不需要反复读取 21 章完整 Agent 设计。
```

前置任务：

```text
P4 已通过复审。
```

必须阅读：

```text
doc/21_Agent系统设计.md 的第 1-8 章和 AgentObservation / AgentIntent / AgentCommandAdapter 小节。
doc/23_工程实现路线图设计.md 的 P5 表格。
doc/30_P5Agent最小定义与观察任务卡设计.md 的第 1-10 章。
backend/dev_notes/command.md。
backend/dev_notes/pipeline.md。
```

禁止阅读：

```text
前端资料。
HTTP / WebSocket 资料。
强化学习算法资料。
完整战斗 AI 资料。
```

允许修改：

```text
context_packs/agent_p5.md
backend/dev_notes/agent.md
```

禁止修改：

```text
backend/src/rules/
backend/src/pipeline/
backend/src/state/
backend/src/replay/
backend/src/protocol/
```

实现要求：

```text
agent_p5.md 写明 P5 只做 Agent 最小闭环。
agent_p5.md 写明禁止 Agent 直接改 GameState / 调 Resolver。
agent_p5.md 写明 P5 允许读取哪些头文件。
agent_p5.md 写明每个任务的验收命令。
backend/dev_notes/agent.md 写明 P5 目录、边界、测试入口。
```

测试要求：

```text
只做文档扫描。
```

验收命令：

```bash
rg -n "P5|Agent|CommandEnvelope|Observation|Intent|禁止|验收" context_packs/agent_p5.md backend/dev_notes/agent.md
```

失败时处理：

```text
如果上下文包要求实现完整调度器，删除该要求。
如果上下文包要求读取前端或网络资料，删除该要求。
```

## 12. TASK-P5-001：创建 agent 模块骨架

任务名称：创建 Agent 模块目录与 CMake 空目标。

任务目标：

```text
让 agent 模块可以独立编译和测试，但不实现完整决策逻辑。
```

前置任务：

```text
TASK-P5-000。
```

必须阅读：

```text
context_packs/agent_p5.md。
backend/CMakeLists.txt。
backend/tests/CMakeLists.txt。
backend/include/pathfinder/foundation/id.h。
backend/include/pathfinder/command/types.h。
```

禁止阅读：

```text
AI 算法资料。
前端资料。
HTTP / WebSocket 资料。
```

允许修改：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/include/pathfinder/agent/README.md
backend/include/pathfinder/agent/types.h
backend/src/agent/types.cpp
backend/tests/unit/agent/agent_smoke_test.cpp
backend/dev_notes/agent.md
```

禁止修改：

```text
backend/src/rules/
backend/src/pipeline/
backend/src/state/
backend/src/command/ 已有行为语义
```

实现要求：

```text
新增 pathfinder_agent 静态库。
新增 agent smoke 测试。
agent README 写明：P5 只定义数据契约和适配器，不做完整 AI。
CMake 链接 foundation 和 command，暂不链接 pipeline。
```

测试要求：

```text
agent smoke 测试通过。
P0-P4 全量测试仍通过。
```

验收命令：

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R agent --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

失败时处理：

```text
如果 smoke 测试为了通过调用 RulePipeline，删除，骨架阶段不做规则执行。
```

## 13. TASK-P5-002：实现 Agent 基础枚举

任务名称：实现 Agent 基础枚举和字符串转换。

任务目标：

```text
固定 Agent 的尺度、认知层级、具身形式、控制器类型和意图类型。
```

前置任务：

```text
TASK-P5-001。
```

必须阅读：

```text
context_packs/agent_p5.md。
backend/include/pathfinder/agent/types.h。
backend/include/pathfinder/command/types.h。
backend/include/pathfinder/foundation/result.h。
```

允许修改：

```text
backend/include/pathfinder/agent/types.h
backend/src/agent/types.cpp
backend/tests/unit/agent/agent_types_test.cpp
backend/dev_notes/agent.md
```

禁止修改：

```text
backend/include/pathfinder/command/types.h，除非 P5 任务明确要求。
backend/include/pathfinder/foundation/id.h，Agent 专用 ID 放在 agent/types.h。
```

实现要求：

```text
实现 AgentScale。
实现 AgentCognitionBand。
实现 AgentEmbodiment。
实现 AgentControllerType。
实现 AgentControlAuthority。
实现 AgentIntentType。
在 agent/types.h 中定义 AgentIdTag / AgentDefinitionIdTag / AgentId / AgentDefinitionId，不修改 foundation/id.h。
为每个枚举实现 toString。
为每个枚举实现 fromString。
Unknown 只能作为默认未设置值，validateBasic 中必须拒绝。
fromString 遇到非法字符串必须返回错误，不要静默返回 Unknown。
```

测试要求：

```text
每个枚举合法值可以往返转换。
非法字符串返回错误。
Unknown 的 toString 明确返回 unknown。
大小写不匹配返回错误，保持配置严格。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_types --output-on-failure
```

失败时处理：

```text
如果 fromString("unknown") 被当成合法业务输入，改为返回错误或只允许内部默认，不允许配置输入。
```

## 14. TASK-P5-003：实现 AgentDefinition / AgentProfile

任务名称：实现智能体定义和画像。

任务目标：

```text
让蜘蛛、狼、先驱者、族群、高级文明都能用统一模板表达。
```

前置任务：

```text
TASK-P5-002。
```

必须阅读：

```text
context_packs/agent_p5.md。
backend/include/pathfinder/foundation/id.h。
backend/include/pathfinder/foundation/result.h。
backend/include/pathfinder/agent/types.h。
```

允许修改：

```text
backend/include/pathfinder/agent/definition.h
backend/src/agent/definition.cpp
backend/tests/unit/agent/agent_definition_test.cpp
backend/dev_notes/agent.md
```

禁止修改：

```text
backend/src/config/。
backend/src/rules/。
```

实现要求：

```text
定义 AgentDefinition。
定义 AgentProfile。
实现 validateBasic。
提供测试夹具：createSpiderDefinition、createWolfDefinition、createPioneerDefinition、createAlienCivilizationDefinition。
夹具可以放在测试文件或 agent test fixture，不要放进正式内容配置。
```

测试要求：

```text
合法 pioneer definition 通过。
合法 spider definition 通过。
合法 wolf definition 通过。
合法 alien civilization definition 通过。
缺 definition_id 失败。
scale Unknown 失败。
cognition_band Unknown 失败。
embodiment Unknown 失败。
profile 数值小于 0 失败。
profile 数值大于 1 失败。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_definition --output-on-failure
```

失败时处理：

```text
如果为了定义高级文明而新增文明玩法逻辑，回滚该部分。P5 只定义模板。
```

## 15. TASK-P5-004：实现 AgentBinding

任务名称：实现 Agent 与规则世界实体的绑定。

任务目标：

```text
明确 Agent 控制哪个 Actor 或 Actor 集合，为后续玩家、AI、族群控制打地基。
```

前置任务：

```text
TASK-P5-003。
```

必须阅读：

```text
context_packs/agent_p5.md。
backend/include/pathfinder/foundation/id.h。
backend/include/pathfinder/state/actor_state.h。
```

允许修改：

```text
backend/include/pathfinder/agent/binding.h
backend/src/agent/binding.cpp
backend/tests/unit/agent/agent_binding_test.cpp
backend/dev_notes/agent.md
```

禁止修改：

```text
backend/src/state/actor_state.cpp。
backend/src/state/game_state.cpp。
```

实现要求：

```text
定义 AgentBinding。
定义 AgentControlAuthority。
实现 validateBasic。
支持 SingleActor / MultiActor / TribeAuthority 的最小字段。
P5 不检查 actor 是否存在，只检查 ID 和重复项。
```

测试要求：

```text
单 actor binding 通过。
多 actor binding 通过。
缺 agent_id 失败。
缺 primary_actor_id 失败。
authority Unknown 失败。
controlled_actor_ids 重复失败。
controlled_actor_ids 非空但不包含 primary_actor_id 失败。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_binding --output-on-failure
```

失败时处理：

```text
如果 Binding 直接持有 GameState 指针或引用，删除。
```

## 16. TASK-P5-005：实现 AgentObservation 数据契约

任务名称：实现智能体观察结构。

任务目标：

```text
固定 Agent 能看到的信息形态，避免 AI 直接读取真实世界隐藏信息。
```

前置任务：

```text
TASK-P5-004。
```

必须阅读：

```text
context_packs/agent_p5.md。
doc/11_认知状态设计.md 的主观认知原则。
backend/include/pathfinder/cognition/cognition_state.h。
backend/include/pathfinder/event/event_record.h。
```

允许修改：

```text
backend/include/pathfinder/agent/observation.h
backend/src/agent/observation.cpp
backend/tests/unit/agent/agent_observation_test.cpp
backend/dev_notes/agent.md
```

禁止修改：

```text
backend/src/cognition/。
backend/src/event/。
backend/src/state/。
```

实现要求：

```text
定义 AgentObservation。
定义 AgentObservedObject。
定义 AgentObservedThreat。
定义 AgentObservedKnowledge。
实现 validateBasic。
对象观察字段只表达：object_id、known_edible_confidence、known_usable_confidence、risk_confidence、evidence_count、can_teach_hint。
威胁观察字段只表达：source_id、threat_type、severity、confidence。
知识观察字段只表达：knowledge_id、claim_type、confidence、evidence_count、teachable_hint。
不允许出现 visual_reveal、shape、color、mosaic、ObjectRevealLevel 等视觉揭示概念。
```

测试要求：

```text
合法 observation 通过。
缺 agent_id 失败。
缺 observer_actor_id 失败。
confidence 小于 0 失败。
confidence 大于 1 失败。
object 观察可以表示可食用置信度。
threat 观察可以表示 fire danger。
knowledge 观察可以表示可传授提示。
测试源码中不得出现 ObjectRevealLevel。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_observation --output-on-failure
rg -n "ObjectRevealLevel|visual_reveal|mosaic|shape|color" backend/include/pathfinder/agent backend/src/agent backend/tests/unit/agent && exit 1 || true
```

失败时处理：

```text
如果 Observation 里出现形态、颜色、马赛克揭示等级，删除并改回认知层字段。
```

## 17. TASK-P5-006：实现 AgentActionSpace

任务名称：实现智能体动作空间。

任务目标：

```text
让 Agent 在决策前知道当前可以选择哪些候选动作，以及哪些动作当前能转成命令。
```

前置任务：

```text
TASK-P5-005。
```

必须阅读：

```text
context_packs/agent_p5.md。
backend/include/pathfinder/command/types.h。
backend/include/pathfinder/command/target.h。
backend/include/pathfinder/agent/types.h。
```

允许修改：

```text
backend/include/pathfinder/agent/action_space.h
backend/src/agent/action_space.cpp
backend/tests/unit/agent/agent_action_space_test.cpp
backend/dev_notes/agent.md
```

禁止修改：

```text
backend/src/command/。
backend/src/rules/。
```

实现要求：

```text
定义 AgentActionCandidate。
定义 AgentActionSpace。
实现 validateBasic。
支持 command_supported 标记。
支持 required_target_types。
支持 reason_key。
ActionSpaceBuilder 不在 P5 实现，P5 可以在测试中手动构造 ActionSpace。
```

测试要求：

```text
空 ActionSpace 合法。
包含 Eat 候选合法。
包含 AvoidRisk 候选合法。
重复 action_id 失败。
Unknown intent_type 失败。
required_target_types 包含 None 失败。
command_supported=false 的候选不能被 AgentCommandAdapter 转命令。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_action_space --output-on-failure
```

失败时处理：

```text
如果实现 ActionSpaceBuilder 并读取完整 GameState，回滚 Builder，P5 只做数据契约。
```

## 18. TASK-P5-007：实现 AgentIntent / AgentDecision

任务名称：实现智能体意图和最小决策记录。

任务目标：

```text
让 Agent 的一次选择可以被记录、解释，并关联到后续 CommandEnvelope。
```

前置任务：

```text
TASK-P5-006。
```

必须阅读：

```text
context_packs/agent_p5.md。
backend/include/pathfinder/foundation/id.h。
backend/include/pathfinder/command/target.h。
backend/include/pathfinder/agent/action_space.h。
```

允许修改：

```text
backend/include/pathfinder/agent/intent.h
backend/src/agent/intent.cpp
backend/tests/unit/agent/agent_intent_test.cpp
backend/dev_notes/agent.md
```

禁止修改：

```text
backend/src/pipeline/。
backend/src/replay/。
```

实现要求：

```text
定义 AgentIntent。
定义 AgentDecision。
AgentDecision 至少包含 decision_id、agent_id、selected_intent、observation_state_version、action_id、reason_key。
实现 validateBasic。
Wait 类型允许 targets 为空。
Eat / Use / Teach / Combine / Fight 要求 targets 非空。
reason_key 必须非空。
```

测试要求：

```text
合法 Eat intent 通过。
合法 Wait intent 通过且 targets 可为空。
Eat intent targets 为空失败。
confidence 越界失败。
reason_key 为空失败。
AgentDecision 可关联 AgentIntent。
AgentDecision 缺 decision_id 失败。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_intent --output-on-failure
```

失败时处理：

```text
如果 AgentDecision 保存 PipelineResult，删除。P5 决策记录不保存规则执行结果。
```

## 19. TASK-P5-008：实现 AgentCommandAdapter

任务名称：实现 AgentIntent 到 CommandEnvelope 的转换。

任务目标：

```text
让 Agent 能通过统一命令入口发起行为，不能绕过规则管线。
```

前置任务：

```text
TASK-P5-007。
```

必须阅读：

```text
context_packs/agent_p5.md。
backend/include/pathfinder/command/envelope.h。
backend/include/pathfinder/command/action_command.h。
backend/include/pathfinder/command/target.h。
backend/include/pathfinder/command/validation.h。
backend/include/pathfinder/agent/intent.h。
```

允许修改：

```text
backend/include/pathfinder/agent/command_adapter.h
backend/src/agent/command_adapter.cpp
backend/tests/unit/agent/agent_command_adapter_test.cpp
backend/dev_notes/agent.md
```

禁止修改：

```text
backend/src/pipeline/。
backend/src/rules/。
backend/src/state/。
backend/src/event/。
```

实现要求：

```text
实现 AgentCommandAdapter::toCommandEnvelope。
Eat 映射为 CommandIntent::Experiment。
Use 映射为 CommandIntent::Experiment。
AvoidRisk / Flee 映射为 CommandIntent::AvoidRisk。
Teach 映射为 CommandIntent::Teach。
Combine 映射为 CommandIntent::Combine。
Fight 映射为 CommandIntent::Fight。
Wait 返回 common_unsupported，不生成命令。
CallGroup 在 P5 返回 common_unsupported 或 command_invalid_argument，不强行映射成 Fight。
生成 command_id / action_id 的策略必须确定性可测。
correlation_id 必须包含 decision_id.value() 或可追踪字符串。
```

ID 生成注意：

```text
P5 不允许研发随手写随机 ID。
推荐用 deterministic helper：makeCommandIdFromDecision(decision_id) 和 makeActionIdFromDecision(decision_id)。
例如 command_agent_decision_001，action_agent_decision_001。
如果使用外部传入 ID 工厂，测试必须固定输出。
```

测试要求：

```text
Eat intent 可以生成 validateBasic 通过的 CommandEnvelope。
CommandEnvelope.source 可以是 Ai / Player / Test。
CommandEnvelope.payload.actor_id 等于 intent.actor_id。
CommandEnvelope.payload.targets 等于 intent.targets。
CommandEnvelope.payload.intent 等于 CommandIntent::Experiment。
Wait intent 返回 unsupported。
CallGroup intent P5 返回 unsupported。
非法 source 返回错误。
Adapter 不依赖 GameState。
Adapter 不链接 pipeline / rules。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_command_adapter --output-on-failure
rg -n "RulePipeline|EatObjectResolver|GameState|StateChange|EventRecord" backend/include/pathfinder/agent/command_adapter.h backend/src/agent/command_adapter.cpp && exit 1 || true
```

失败时处理：

```text
如果 Adapter 调用 RulePipeline，删除调用，改回只返回 CommandEnvelope。
```

## 20. TASK-P5-009：实现 P5 unknown fruit Agent 集成测试

任务名称：实现先驱者吃未知果实的 Agent 到规则管线闭环。

任务目标：

```text
证明 AgentIntent 可以发起 P3 的 unknown_fruit + eat 规则闭环。
```

前置任务：

```text
TASK-P5-008。
```

必须阅读：

```text
context_packs/agent_p5.md。
backend/include/pathfinder/rules/unknown_fruit_fixture.h。
backend/include/pathfinder/pipeline/rule_pipeline.h。
backend/tests/integration/p3/unknown_fruit_eat_flow_test.cpp。
backend/tests/integration/p4/replay_protocol_flow_test.cpp。
```

允许修改：

```text
backend/tests/integration/p5/agent_unknown_fruit_flow_test.cpp
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
```

禁止修改：

```text
backend/src/rules/eat_object_resolver.cpp。
backend/src/pipeline/rule_pipeline.cpp。
backend/src/state/。
```

实现要求：

```text
创建 P5 集成测试目标。
使用 UnknownFruitFixture 创建初始 GameState。
构造 AgentDefinition / AgentBinding / AgentObservation / AgentActionSpace / AgentIntent。
用 AgentCommandAdapter 生成 CommandEnvelope。
测试中把 CommandEnvelope 提交给 RulePipeline。
断言 PipelineResult 成功。
断言 actor_001 hunger 从 80 到 60。
断言 obj_unknown_fruit_001 consumed。
断言 cognition claim confidence/evidence_count 更新。
```

测试要求：

```text
P5 集成测试通过。
P3/P4 集成测试仍通过。
P0-P4 全量测试仍通过。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R "p5|p3|p4" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

失败时处理：

```text
如果为了 P5 测试修改 P3 fixture 或 resolver 行为，回滚 P3 改动。
```

## 21. TASK-P5-010：实现 Spider flee fire 测试级夹具

任务名称：实现蜘蛛遇火逃避的 Agent 表达测试。

任务目标：

```text
证明 MicroCreature Agent 可以通过观察危险产生逃避意图。
```

前置任务：

```text
TASK-P5-008。
```

必须阅读：

```text
context_packs/agent_p5.md。
backend/include/pathfinder/agent/definition.h。
backend/include/pathfinder/agent/observation.h。
backend/include/pathfinder/agent/action_space.h。
backend/include/pathfinder/agent/intent.h。
```

允许修改：

```text
backend/tests/integration/p5/spider_flee_fire_test.cpp
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
```

禁止修改：

```text
backend/src/rules/。
backend/src/pipeline/。
backend/src/state/。
```

实现要求：

```text
创建 spider AgentDefinition：MicroCreature + Reflex 或 Instinct + SingleBody。
创建 fire_danger AgentObservedThreat。
创建包含 Flee / AvoidRisk 的 AgentActionSpace。
创建 Flee AgentIntent。
如果 Flee 可以映射 AvoidRisk，则只测试 CommandEnvelope 基础合法，不提交 RulePipeline。
如果 Adapter 对 Flee 返回 unsupported，则测试 unsupported 是明确错误，不允许静默成功。
```

测试要求：

```text
spider definition validateBasic 通过。
fire danger observation validateBasic 通过。
flee action candidate validateBasic 通过。
flee intent validateBasic 通过。
测试不得新增 flee resolver。
测试不得修改 GameState。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R spider_flee --output-on-failure
rg -n "flee.*resolver|FleeResolver|class.*Flee" backend/src backend/include/pathfinder/rules && exit 1 || true
```

失败时处理：

```text
如果为了蜘蛛测试新增真实逃跑规则，回滚。P5 只做 Agent 表达能力。
```

## 22. TASK-P5-011：实现 Wolf call pack 测试级夹具

任务名称：实现狼呼叫群体的 Agent 表达测试。

任务目标：

```text
证明 SmallCreature Agent 可以表达共同作战或群体协作意图，但不提前实现战斗和族群战争。
```

前置任务：

```text
TASK-P5-008。
```

必须阅读：

```text
context_packs/agent_p5.md。
backend/include/pathfinder/agent/definition.h。
backend/include/pathfinder/agent/binding.h。
backend/include/pathfinder/agent/action_space.h。
backend/include/pathfinder/agent/intent.h。
```

允许修改：

```text
backend/tests/integration/p5/wolf_call_pack_test.cpp
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
```

禁止修改：

```text
backend/src/rules/。
backend/src/pipeline/。
backend/src/state/。
backend/src/command/types.cpp，除非 P5 后续明确新增 CommandIntent。
```

实现要求：

```text
创建 wolf AgentDefinition：SmallCreature + Instinct 或 Associative + SingleBody。
AgentProfile sociality / cooperation 应高于 spider。
创建 CallGroup AgentActionCandidate，command_supported=false。
创建 CallGroup AgentIntent。
AgentCommandAdapter 对 CallGroup 返回 unsupported，直到后续正式设计 group command。
测试证明 CallGroup 可以被 Agent 层表达，但不能被错误提交到 RulePipeline。
```

测试要求：

```text
wolf definition validateBasic 通过。
wolf binding validateBasic 通过。
call_group candidate validateBasic 通过。
call_group intent validateBasic 通过。
Adapter 返回明确 unsupported。
测试不得新增 group combat resolver。
测试不得新增 tribe split 逻辑。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R wolf_call_pack --output-on-failure
rg -n "tribe_split|pack_combat|GroupCombat|WarResolver" backend/src backend/include/pathfinder && exit 1 || true
```

失败时处理：

```text
如果为了狼测试实现战斗结算，回滚。共同作战真实规则留到战斗/族群阶段。
```

## 23. TASK-P5-012：P5 边界审计与文档更新

任务名称：执行 P5 全量验证和边界审计。

任务目标：

```text
确保 P5 只完成 Agent 最小闭环，没有越界实现完整 AI、前端、战斗或存档。
```

前置任务：

```text
TASK-P5-009。
TASK-P5-010。
TASK-P5-011。
```

必须阅读：

```text
context_packs/agent_p5.md。
doc/30_P5Agent最小定义与观察任务卡设计.md。
backend/dev_notes/agent.md。
```

允许修改：

```text
backend/dev_notes/agent.md
doc/review/P5Agent最小定义与观察审核报告.md
```

禁止修改：

```text
任何生产代码，除非审计发现 P5 自身阻塞 bug。
```

审计要求：

```text
确认 agent 模块不依赖 frontend / HTTP / WebSocket。
确认 AgentCommandAdapter 不依赖 RulePipeline / Resolver / GameState。
确认 P5 未新增完整调度器代码文件。
确认 P5 未新增强化学习环境代码文件。
确认 P5 未新增战斗、族群分裂、战争 resolver。
确认 Observation 中没有视觉揭示概念。
确认 P5 unknown fruit 集成测试通过。
确认 P0-P4 全量测试仍通过。
```

验收命令：

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent|p5|p3|p4" --output-on-failure
ctest --test-dir build/backend --output-on-failure
find backend -type d \( -path '*/frontend*' -o -path '*/http*' -o -path '*/websocket*' \) -print
rg -n "RulePipeline|EatObjectResolver|GameState|StateChange|EventRecord" backend/include/pathfinder/agent/command_adapter.h backend/src/agent/command_adapter.cpp && exit 1 || true
rg -n "ObjectRevealLevel|visual_reveal|mosaic|shape|color" backend/include/pathfinder/agent backend/src/agent backend/tests/unit/agent backend/tests/integration/p5 && exit 1 || true
rg -n "rl_environment|policy_tree|WarResolver|GroupCombat|tribe_split|SaveManager|WebSocket|HTTP" backend/include/pathfinder/agent backend/src/agent backend/tests/unit/agent backend/tests/integration/p5 && exit 1 || true
```

审核报告必须包含：

```text
测试总数。
失败测试列表。
越界扫描结果。
P5 是否通过。
是否允许进入 P6。
非阻塞建议。
```

失败时处理：

```text
如果只是文档命中禁词，不算生产代码失败，但要说明。
如果生产代码命中禁词，必须修复后再通过。
如果测试为了通过放松 validateBasic，必须回滚。
```

## 24. P5 任务依赖顺序

推荐严格按下面顺序实现：

```text
TASK-P5-000：上下文包。
TASK-P5-001：模块骨架。
TASK-P5-002：基础枚举。
TASK-P5-003：AgentDefinition / AgentProfile。
TASK-P5-004：AgentBinding。
TASK-P5-005：AgentObservation。
TASK-P5-006：AgentActionSpace。
TASK-P5-007：AgentIntent / AgentDecision。
TASK-P5-008：AgentCommandAdapter。
TASK-P5-009：unknown fruit Agent 集成测试。
TASK-P5-010：spider flee fire 测试级夹具。
TASK-P5-011：wolf call pack 测试级夹具。
TASK-P5-012：边界审计与审核报告。
```

不能并行的任务：

```text
P5-002 必须在所有 agent 类型任务前完成。
P5-008 必须在 P5-009/P5-010/P5-011 前完成。
P5-012 必须最后做。
```

可以并行的任务：

```text
P5-003 和 P5-004 在 P5-002 后可以并行。
P5-005 和 P5-006 在 P5-002 后可以并行。
P5-010 和 P5-011 在 P5-008 后可以并行。
```

## 25. P5 完成标准

P5 完成必须同时满足：

```text
agent 模块能独立编译。
所有 Agent 枚举有严格字符串转换测试。
AgentDefinition 能表达 spider / wolf / pioneer / alien civilization。
AgentObservation 不包含视觉揭示概念。
AgentActionSpace 能表达 supported 和 unsupported 候选。
AgentIntent 能表达 Eat / Flee / CallGroup。
AgentCommandAdapter 能把 Eat 转为合法 CommandEnvelope。
AgentCommandAdapter 不执行规则。
P5 unknown fruit 集成测试走 RulePipeline 成功。
Spider / Wolf 测试只证明 Agent 表达，不新增玩法 resolver。
P0-P4 全量测试仍通过。
doc/review/P5Agent最小定义与观察审核报告.md 已生成。
```

## 26. P5 后续进入 P6 的条件

只有 P5 通过后，P6 才能设计或实现：

```text
ObservationBuilder：从 GameState / Cognition / Event 构建观察。
ActionSpaceBuilder：根据观察和能力生成动作空间。
AgentPolicy：规则策略、脚本策略、测试策略。
Agent 决策轨迹：Observation / Action / Reward / Done。
Replay 与 AgentDecisionRecord 的正式对接。
更完整的群体协作、共同作战和族群行为。
```

P6 不应提前做：

```text
神经网络训练。
大型 LLM 决策。
真实多人联网。
完整战斗 AI。
```

P5 的正确结束状态是：

```text
Agent 会表达意图。
Command 会承接意图。
Pipeline 会结算命令。
Replay / Protocol 未来可以记录和输出结果。
智能程度留给后续阶段。
```
