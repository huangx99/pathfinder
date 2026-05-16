# 32_P7Agent运行时最小闭环任务卡设计

设计日期：2026-05-17  
阶段：P7  
阶段名称：AgentRuntime 最小调度闭环  
结论：P7 只实现最小运行时，不实现完整 AI、不实现 H5、不实现传播/族群/文明。

## 1. P7 要解决什么问题

P5 已经有 Agent 数据契约和 `AgentCommandAdapter`。  
P6 已经有 `ObservationBuilder` 和 `ActionSpaceBuilder`。  
但现在仍然缺一个稳定的运行时编排层：

```text
GameState + VisibilityInput
-> ObservationBuilder
-> AgentObservation
-> ActionSpaceBuilder
-> AgentActionSpace
-> 最小 Policy 选择候选动作
-> AgentIntent / AgentDecision
-> AgentCommandAdapter
-> CommandEnvelope
-> RulePipeline
-> PipelineResult
```

P7 就是补上这个“最小 AgentRuntime”。

P7 的核心目标不是让 Agent 变聪明，而是让 Agent 决策链路从测试手写流程变成可复用、可测试、可复盘的运行时流程。

## 2. P7 的工程定位

P7 是 Agent 系统从“数据契约 + Builder”进入“运行时编排”的第一步。

P7 必须保持三层分离：

```text
Builder 层：负责把世界状态投影成主观观察和动作空间。
Policy 层：只看 observation / action_space，选择一个候选动作。
Runtime 层：按固定流程调用 builder、policy、adapter、pipeline。
```

P7 不能把这三层混在一起。

## 3. P7 与现有阶段关系

P7 依赖：

```text
P0 foundation：Result、ErrorDetail、StrongId、Tick、StateVersion、RandomSeed。
P1 command：CommandEnvelope、CommandSource、ActionTarget。
P2 state/event/pipeline：GameState、PipelineContext、PipelineResult、RulePipeline。
P3 rules fixture：UnknownFruitFixture，仅用于集成测试。
P4 replay/protocol：本阶段不新增依赖，只跑回归测试。
P5 agent core：AgentBinding、AgentIntent、AgentDecision、AgentCommandAdapter。
P6 builders：ObservationBuilder、ActionSpaceBuilder、VisibilityInput。
```

P7 不依赖：

```text
H5 / HTTP / WebSocket。
SaveManager。
完整 ConfigRegistry。
表达式引擎。
传播系统。
族群状态系统。
文明能力系统。
强化学习环境。
LLM / 外部 AI 服务。
真实 flee resolver / group combat resolver / war resolver。
```

说明：`doc/23_工程实现路线图设计.md` 中早期写过 “P6 H5 / P7 propagation”。当前实现路线已经因为 Agent 被拆成 P5/P6/P7 而细化，本任务卡是当前 P7 的执行权威。H5、传播、族群、文明仍然保留在后续阶段，不在 P7 实现。

## 4. P7 最小闭环定义

### 4.1 unknown fruit 自动闭环

输入：

```text
UnknownFruitFixture::createInitialState()
AgentBinding: agent_player_001 -> actor_001
VisibilityInput: obj_unknown_fruit_001 可见
Policy: FirstSupportedPolicy
Runtime submit allowlist: eat
```

流程：

```text
Runtime 构建 observation。
Runtime 构建 action_space。
Policy 选择 Eat candidate。
Policy 生成 AgentDecision。
Adapter 转 CommandEnvelope。
Runtime 只在 action_id 允许提交时调用 RulePipeline。
RulePipeline 执行 eat 规则。
状态变化、事件、认知更新保持 P3/P6 语义稳定。
```

验收：

```text
actor_001.hunger 从 80 变 60。
obj_unknown_fruit_001 变 consumed。
CognitionState 生成一条认知 claim。
Runtime trace 能看到 observation/action_space/decision/command/pipeline 阶段。
```

### 4.2 threat-only 不误提交闭环

输入：

```text
VisibilityInput: Fire threat seed
无可见 fruit object
ActionSpaceBuilder 生成 Flee candidate
```

P7 要求：

```text
Runtime 可以生成 Flee decision。
Runtime 可以选择不提交 pipeline。
Runtime 不能把 Flee 提交给当前只有 eat 规则的 RulePipeline，并把 empty success 当成真实逃跑成功。
```

验收：

```text
没有新增 FleeResolver。
没有状态变化。
Runtime status 为 decision_only / command_created / submit_skipped 之一，而不是 pipeline_succeeded。
```

## 5. P7 必须先修正的 P6 结构缺口

P6 暴露出一个运行时缺口：`AgentActionCandidate` 当前只有 `required_target_types`，没有候选动作对应的具体目标；同时 `action_id` 在 P6 中被用成候选唯一 ID，但 `AgentCommandAdapter` 和 `RulePipeline` 需要语义 action id，例如 `eat`。

P7 必须补上这个缺口，否则 Runtime 无法从 candidate 自动生成正确 intent。

### 5.1 AgentActionCandidate 新语义

P7 后：

```cpp
struct AgentActionCandidate {
    ActionId action_id;                 // 候选 ID，可保持 eat_obj_xxx / flee_xxx
    ActionId command_action_id;         // 真正进入 CommandEnvelope 的语义 action id，例如 eat
    AgentIntentType intent_type;
    vector<ActionTargetType> required_target_types;
    vector<ActionTarget> suggested_targets;
    bool command_supported;
    string reason_key;
};
```

规则：

```text
action_id：用于 trace、候选稳定排序、调试，可以是 eat_obj_unknown_fruit_001。
command_action_id：用于 AgentIntent.action_id 和 CommandEnvelope.payload.action_id。
如果 command_action_id 为空，Policy 可以回退使用 action_id，但 P7 的 Eat candidate 必须显式填写 command_action_id=eat。
suggested_targets：用于 Policy 构造 AgentIntent.targets。
required_target_types：仍保留，用于校验 suggested_targets 是否满足类型要求。
```

### 5.2 为什么不直接把 action_id 改成 eat

不推荐直接把所有 Eat candidate 的 `action_id` 改成 `eat`，原因：

```text
同一 observation 里可能有多个可吃对象。
多个 candidate 都叫 eat 会降低 trace 可读性。
候选动作需要可排序、可定位、可调试。
命令语义 action 和候选实例 ID 是两件事。
```

因此 P7 采用双字段：

```text
action_id = candidate instance id
command_action_id = executable command semantic id
```

## 6. P7 新增核心类型

### 6.1 AgentPolicyRequest

文件建议：`backend/include/pathfinder/agent/policy.h`

```cpp
struct AgentPolicyRequest {
    AgentBinding binding;
    AgentObservation observation;
    AgentActionSpace action_space;
    Tick decision_tick;
    RandomSeed random_seed;
    std::string policy_reason_prefix;

    Result<void> validateBasic() const;
};
```

约束：

```text
不得包含 GameState 指针。
不得包含 ObjectStore 指针。
不得包含 RulePipeline 指针。
不得包含 hidden/debug truth。
```

### 6.2 AgentPolicy 接口

```cpp
class AgentPolicy {
public:
    virtual ~AgentPolicy() = default;
    virtual Result<AgentDecision> decide(const AgentPolicyRequest& request) const = 0;
};
```

P7 只允许实现一个最小策略：

```text
FirstSupportedPolicy
```

不允许实现：

```text
BehaviorTreePolicy。
GOAPPolicy。
UtilityPolicy。
NeuralPolicy。
LLMPolicy。
RLPolicy。
复杂目标系统。
```

### 6.3 FirstSupportedPolicy

职责：

```text
从 action_space.candidates 中按顺序选择第一个 command_supported=true 的 candidate。
如果 request 中配置 submit allowlist，则优先选择 command_action_id 在 allowlist 里的 candidate。
如果没有可选 candidate，返回 error 或 no_decision 状态，不伪造 Wait command。
```

生成 `AgentIntent` 规则：

```text
intent.agent_id = binding.agent_id。
intent.actor_id = binding.primary_actor_id。
intent.decision_id = deterministic id。
intent.intent_type = candidate.intent_type。
intent.action_id = candidate.command_action_id 非空时使用它，否则使用 candidate.action_id。
intent.targets = candidate.suggested_targets。
intent.command_supported_snapshot = candidate.command_supported。
intent.reason_key = candidate.reason_key。
intent.confidence = 0.5 或 0.8，P7 固定常量即可。
```

生成 `AgentDecision` 规则：

```text
decision.decision_id = intent.decision_id。
decision.agent_id = intent.agent_id。
decision.selected_intent = intent。
decision.observation_state_version = observation.state_version。
decision.action_id = intent.action_id。
decision.reason_key = intent.reason_key。
```

确定性 ID 建议：

```text
decision_<agent_id>_<tick>_<candidate_index>
```

注意：ID 必须符合 `isValidIdString`，不要使用空格、冒号、斜杠、中文。

### 6.4 AgentRuntimeOptions

文件建议：`backend/include/pathfinder/agent/runtime_types.h`

```cpp
struct AgentRuntimeOptions {
    bool include_cognition_claims = true;
    bool include_explore_candidates = true;
    bool include_wait_candidate = false;
    bool build_command = true;
    bool submit_to_pipeline = true;
    std::vector<ActionId> submit_action_allowlist;
};
```

规则：

```text
P7 如果 submit_to_pipeline=true，submit_action_allowlist 必须非空。
P7 unknown fruit 测试传入 allowlist = [eat]。
Flee / CallGroup 不在 allowlist 时不能提交 RulePipeline。
后续有 ActionRegistry 后，可替换 allowlist 机制，但 Runtime 流程不需要推翻。
```

### 6.5 AgentRuntimeStatus

```cpp
enum class AgentRuntimeStatus {
    Unknown,
    ObservationBuilt,
    ActionSpaceBuilt,
    DecisionMade,
    CommandCreated,
    SubmitSkipped,
    PipelineSucceeded,
    PipelineFailed,
    NoDecision,
    Failed
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值，不应作为有效结果 |
| ObservationBuilt | 已构建观察 | 只完成观察阶段 |
| ActionSpaceBuilt | 已构建动作空间 | 完成观察和动作空间 |
| DecisionMade | 已产生决策 | Policy 已给出决策 |
| CommandCreated | 已生成命令 | Adapter 已生成 CommandEnvelope |
| SubmitSkipped | 跳过提交 | 因 allowlist 或配置未提交管线 |
| PipelineSucceeded | 管线成功 | RulePipeline 执行成功且状态为 succeeded |
| PipelineFailed | 管线失败 | RulePipeline 返回 failed 或 errors |
| NoDecision | 无决策 | 没有可选候选动作 |
| Failed | 失败 | 构建、校验或适配失败 |

### 6.6 AgentRuntimeTrace

```cpp
struct AgentRuntimeTrace {
    std::vector<std::string> phase_keys;
    std::vector<std::string> reason_keys;
    int selected_candidate_index = -1;
    ActionId selected_candidate_id;
    ActionId selected_command_action_id;
    bool command_created = false;
    bool pipeline_submitted = false;
};
```

Trace 只用于调试和测试，不作为规则权威。

### 6.7 AgentTickRequest

```cpp
struct AgentTickRequest {
    AgentBinding binding;
    GameState* state = nullptr;
    VisibilityInput visibility;
    Tick issued_tick;
    RandomSeed random_seed;
    CommandSource command_source = CommandSource::Ai;
    AgentRuntimeOptions options;

    Result<void> validateBasic() const;
};
```

约束：

```text
state 必须非空。
binding 必须合法。
visibility 必须合法。
command_source 不能为 Unknown。
如果 options.submit_to_pipeline=true，则 submit_action_allowlist 必须非空。
```

### 6.8 AgentTickResult

```cpp
struct AgentTickResult {
    AgentRuntimeStatus status = AgentRuntimeStatus::Unknown;
    AgentObservation observation;
    AgentActionSpace action_space;
    AgentDecision decision;
    std::optional<CommandEnvelope> command;
    std::optional<PipelineResult> pipeline_result;
    AgentRuntimeTrace trace;
    std::vector<ErrorDetail> warnings;

    Result<void> validateBasic() const;
};
```

注意：

```text
P7 可以使用 std::optional。
如果现有工程风格不想引入 optional，也可以用 bool has_command / has_pipeline_result。
但必须避免用默认空 CommandEnvelope 冒充已经生成命令。
```

### 6.9 AgentRuntime

文件建议：`backend/include/pathfinder/agent/runtime.h`

```cpp
class AgentRuntime {
public:
    explicit AgentRuntime(const AgentPolicy& policy);

    Result<AgentTickResult> tickOne(const AgentTickRequest& request) const;

private:
    const AgentPolicy& policy_;
};
```

`tickOne` 流程：

```text
1. validate AgentTickRequest。
2. ObservationBuilder.build。
3. ActionSpaceBuilder.build。
4. policy.decide。
5. AgentCommandAdapter.toCommandEnvelope。
6. 如果 submit_to_pipeline=false，返回 CommandCreated。
7. 如果 action_id 不在 submit_action_allowlist，返回 SubmitSkipped。
8. 构造 PipelineContext。
9. RulePipeline.execute。
10. 根据 PipelineResult.status 设置 PipelineSucceeded / PipelineFailed。
11. 返回 AgentTickResult。
```

## 7. P7 不做什么

P7 禁止实现：

```text
完整多 Agent 调度器。
AgentService。
Agent registry / AgentStore。
AgentState 持久化。
目标系统 AgentGoal。
复杂策略系统。
行为树 / GOAP / 效用 AI / 神经网络 / 强化学习。
LLM 接入。
PerceptionSystem / 地图视野系统。
FleeResolver。
GroupCombat / WarResolver / tribe_split。
传播、族群、文明规则。
H5 / HTTP / WebSocket。
SaveManager。
```

P7 也禁止：

```text
Policy 读取 GameState。
Policy 读取 ObjectStore / ActorStore。
Policy 调用 RulePipeline。
Policy 根据真实 edible_profile 做选择。
Runtime 绕过 AgentCommandAdapter 直接构造 CommandEnvelope。
Runtime 直接修改 GameState。
Runtime 把 RulePipeline empty success 当成真实动作成功。
为了让 Flee 测试通过新增真实逃跑结算。
为了让 CallGroup 测试通过新增群体战斗或族群系统。
```

## 8. 文件范围

### 8.1 允许新增

```text
backend/include/pathfinder/agent/policy.h
backend/include/pathfinder/agent/runtime_types.h
backend/include/pathfinder/agent/runtime.h
backend/src/agent/policy.cpp
backend/src/agent/runtime_types.cpp
backend/src/agent/runtime.cpp
backend/tests/unit/agent/agent_policy_test.cpp
backend/tests/unit/agent/agent_runtime_types_test.cpp
backend/tests/unit/agent/agent_runtime_test.cpp
backend/tests/integration/p7/agent_runtime_unknown_fruit_flow_test.cpp
backend/tests/integration/p7/agent_runtime_threat_no_submit_test.cpp
context_packs/agent_p7.md
```

### 8.2 允许修改

```text
backend/include/pathfinder/agent/action_space.h
backend/src/agent/action_space.cpp
backend/src/agent/action_space_builder.cpp
backend/tests/unit/agent/agent_action_space_test.cpp
backend/tests/unit/agent/agent_action_space_builder_test.cpp
backend/tests/integration/p6/agent_builder_unknown_fruit_flow_test.cpp
backend/tests/integration/p6/spider_fire_action_space_test.cpp
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
```

### 8.3 禁止修改

```text
backend/src/rules/
backend/include/pathfinder/rules/
backend/src/object/
backend/include/pathfinder/object/
backend/src/pipeline/rule_pipeline.cpp，除非发现 P7 无法绕开的 P0-P6 阻塞 bug。
frontend/
任何 HTTP / WebSocket / SaveManager 相关目录。
```

## 9. 必读文档

P7 实现 AI 必须读取：

```text
doc/32_P7Agent运行时最小闭环任务卡设计.md
context_packs/agent_p6.md
doc/21_Agent系统设计.md 的 16-18 节
doc/31_P6观察与动作空间构建任务卡设计.md 的 P6 边界和 TASK-P6-007/008/009
doc/22_测试策略设计.md 中单元测试、集成测试、边界扫描原则
backend/dev_notes/agent.md
```

最多再读取：

```text
backend/include/pathfinder/agent/action_space.h
backend/include/pathfinder/agent/intent.h
backend/include/pathfinder/agent/command_adapter.h
backend/include/pathfinder/agent/builder_types.h
backend/include/pathfinder/pipeline/context.h
backend/include/pathfinder/pipeline/result.h
```

禁止为了 P7 一次性读取：

```text
01-20 全部设计文档。
传播、族群、文明完整设计。
H5 / 协议完整设计。
存档完整设计。
```

## 10. TASK-P7-000：上下文包

任务名称：创建 P7 上下文包。

允许新增：

```text
context_packs/agent_p7.md
```

上下文包必须包含：

```text
P7 目标。
P7 允许实现类。
P7 禁止实现类。
P7 文件范围。
P7 验收命令。
P7 与 P5/P6 的关系。
P7 action candidate 语义修正。
P7 submit allowlist 原则。
```

验收：

```bash
rg -n "P7|AgentRuntime|FirstSupportedPolicy|submit_action_allowlist|command_action_id" context_packs/agent_p7.md
```

## 11. TASK-P7-001：修正 ActionCandidate 可执行语义

任务名称：让 ActionSpace candidate 可以被 Runtime 自动转成 Intent。

允许修改：

```text
backend/include/pathfinder/agent/action_space.h
backend/src/agent/action_space.cpp
backend/src/agent/action_space_builder.cpp
backend/tests/unit/agent/agent_action_space_test.cpp
backend/tests/unit/agent/agent_action_space_builder_test.cpp
backend/tests/integration/p6/agent_builder_unknown_fruit_flow_test.cpp
backend/tests/integration/p6/spider_fire_action_space_test.cpp
```

实现要求：

```text
AgentActionCandidate 增加 command_action_id。
AgentActionCandidate 增加 suggested_targets。
validateBasic 校验 suggested_targets 中每个 target 合法。
如果 suggested_targets 非空，target_type 应该覆盖 required_target_types 的要求。
Eat candidate: action_id = eat_<object_id>，command_action_id = eat，suggested_targets = object target。
Explore candidate: command_action_id 可以为 explore，command_supported=false。
Flee candidate: action_id = flee_<source_id>，command_action_id 可以为 flee 或保持当前候选 ID，但 P7 不提交。
CallGroup candidate: command_supported=false。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R "agent_action_space|agent_action_space_builder|p6" --output-on-failure
```

必须新增/调整测试：

```text
Eat candidate 带 object suggested_target。
Eat candidate.command_action_id == eat。
多 object 时两个 Eat candidate 的 action_id 不同，但 command_action_id 都是 eat。
Flee candidate 带 threat source target。
P6 unknown fruit 集成测试不再手写 target 来源，应能从 candidate.suggested_targets 复制。
```

失败时处理：

```text
如果为了省事把所有 Eat candidate.action_id 改成 eat，回滚。
如果 Policy 需要从 reason_key 字符串解析 object id，回滚。
```

## 12. TASK-P7-002：实现最小 Policy 接口和 FirstSupportedPolicy

任务名称：实现可替换策略接口和一个确定性最小策略。

允许新增：

```text
backend/include/pathfinder/agent/policy.h
backend/src/agent/policy.cpp
backend/tests/unit/agent/agent_policy_test.cpp
```

允许修改：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

实现要求：

```text
定义 AgentPolicyRequest。
定义 AgentPolicy 接口。
实现 FirstSupportedPolicy。
Policy 不读取 GameState。
Policy 不调用 RulePipeline。
Policy 不调用 AgentCommandAdapter。
Policy 只输出 AgentDecision。
Policy 使用 candidate.command_action_id / suggested_targets 构造 AgentIntent。
Policy 生成确定性 decision_id。
Policy 没有可选 candidate 时返回 error，不伪造成功。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_policy --output-on-failure
rg -n "GameState|object_store|actor_store|RulePipeline|EatObjectResolver|edible_profile|hunger_delta|health_delta|effect_kind" backend/include/pathfinder/agent/policy.h backend/src/agent/policy.cpp backend/tests/unit/agent/agent_policy_test.cpp && exit 1 || true
```

测试要求：

```text
选择第一个 command_supported=true candidate。
跳过 command_supported=false candidate。
使用 candidate.suggested_targets 生成 intent.targets。
使用 candidate.command_action_id 生成 intent.action_id。
无 candidate 返回 error。
无 supported candidate 返回 error。
decision validateBasic 通过。
```

## 13. TASK-P7-003：实现 Runtime 类型

任务名称：定义 P7 runtime request/result/trace/status。

允许新增：

```text
backend/include/pathfinder/agent/runtime_types.h
backend/src/agent/runtime_types.cpp
backend/tests/unit/agent/agent_runtime_types_test.cpp
```

实现要求：

```text
AgentRuntimeStatus。
AgentRuntimeOptions。
AgentRuntimeTrace。
AgentTickRequest。
AgentTickResult。
validateBasic 覆盖空 state、非法 binding、Unknown command_source、submit allowlist 为空等错误。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_runtime_types --output-on-failure
```

测试要求：

```text
默认 options 不提交 pipeline 或者 submit_to_pipeline=true 时必须有 allowlist，二选一必须明确。
request.state=null 返回 error。
command_source=Unknown 返回 error。
submit_to_pipeline=true 且 allowlist 为空返回 error。
status toString/fromString 如果实现，则必须全枚举覆盖。
```

## 14. TASK-P7-004：实现 AgentRuntime build 阶段

任务名称：Runtime 串联 ObservationBuilder 和 ActionSpaceBuilder。

允许新增：

```text
backend/include/pathfinder/agent/runtime.h
backend/src/agent/runtime.cpp
backend/tests/unit/agent/agent_runtime_test.cpp
```

实现要求：

```text
AgentRuntime 构造时注入 const AgentPolicy&。
tickOne 先 validate request。
调用 ObservationBuilder.build。
调用 ActionSpaceBuilder.build。
把 observation/action_space 写入 AgentTickResult。
trace.phase_keys 至少记录 observation_built / action_space_built。
任何 builder 返回 error，Runtime 返回 Result::fail 或 status=Failed，不能继续执行。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_runtime --output-on-failure
```

测试要求：

```text
合法 unknown fruit 输入能构建 observation/action_space。
缺失 object 时 Runtime 返回 error。
consumed object 被 observation 跳过后，action_space 没有 Eat candidate。
```

## 15. TASK-P7-005：Runtime 决策和命令适配

任务名称：Runtime 调用 Policy 并通过 Adapter 生成命令。

允许修改：

```text
backend/src/agent/runtime.cpp
backend/tests/unit/agent/agent_runtime_test.cpp
```

实现要求：

```text
Runtime 调用 policy.decide。
Policy 成功后保存 AgentDecision。
Runtime 调用 AgentCommandAdapter.toCommandEnvelope。
Adapter 成功后保存 CommandEnvelope。
如果 options.build_command=false，Runtime 在 DecisionMade 返回，不生成命令。
如果 Adapter 返回 unsupported，Runtime 返回 SubmitSkipped 或 Failed，不能伪造命令。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_runtime --output-on-failure
```

测试要求：

```text
Eat candidate 能生成 CommandEnvelope。
CommandEnvelope.payload.action_id == eat。
CommandEnvelope.payload.targets 来自 candidate.suggested_targets。
Explore / CallGroup command_supported=false 时不会生成可提交命令。
```

## 16. TASK-P7-006：Runtime 受控提交 RulePipeline

任务名称：Runtime 只把明确允许的命令提交给 RulePipeline。

允许修改：

```text
backend/src/agent/runtime.cpp
backend/tests/unit/agent/agent_runtime_test.cpp
```

实现要求：

```text
如果 submit_to_pipeline=false，返回 CommandCreated。
如果 submit_to_pipeline=true，但 intent.action_id 不在 submit_action_allowlist，返回 SubmitSkipped。
只有通过 allowlist 的命令才能构造 PipelineContext。
PipelineContext.command = CommandEnvelope。
PipelineContext.state_metadata = state.metadata。
PipelineContext.game_state = request.state。
PipelineContext.random_seed = request.random_seed。
RulePipeline.execute 返回 Result error 时，Runtime 返回 error 或 PipelineFailed。
PipelineResult.status=Succeeded 时，Runtime status=PipelineSucceeded。
PipelineResult.status=Failed 时，Runtime status=PipelineFailed。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_runtime --output-on-failure
rg -n "FleeResolver|GroupCombat|WarResolver|tribe_split|pack_combat" backend/include/pathfinder backend/src && exit 1 || true
```

测试要求：

```text
allowlist=[eat] 时 Eat 可以提交。
allowlist 不含 flee 时 Flee 不提交。
submit_to_pipeline=false 时即使命令可构建也不提交。
不能把 PipelineResult::successEmpty 当成 Flee 成功。
```

## 17. TASK-P7-007：unknown fruit runtime 集成测试

任务名称：用 Runtime 代替 P6 手写流程，跑通 unknown fruit。

允许新增：

```text
backend/tests/integration/p7/agent_runtime_unknown_fruit_flow_test.cpp
```

允许修改：

```text
backend/tests/CMakeLists.txt
```

实现要求：

```text
创建 UnknownFruitFixture 初始状态。
创建 AgentBinding。
创建 VisibilityInput，包含 obj_unknown_fruit_001。
创建 FirstSupportedPolicy。
创建 AgentRuntime。
options.submit_to_pipeline=true。
options.submit_action_allowlist=[eat]。
执行 runtime.tickOne。
验证 PipelineSucceeded。
验证 hunger/object/cognition 结果与 P3/P6 一致。
再次 tick consumed object，验证不产生 Eat 提交。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R p7_agent_runtime_unknown_fruit_flow --output-on-failure
```

## 18. TASK-P7-008：threat runtime 不误提交集成测试

任务名称：证明 P7 Runtime 不把 Flee/CallGroup 当成已结算规则。

允许新增：

```text
backend/tests/integration/p7/agent_runtime_threat_no_submit_test.cpp
```

实现要求：

```text
Fire threat seed 生成 Flee candidate。
Runtime 可以 build observation/action_space/decision。
如果 allowlist=[eat]，Flee 不提交 RulePipeline。
Runtime status=SubmitSkipped 或 NoDecision，按实现约定固定一种。
GameState 不发生变化。
Social threat 生成 CallGroup candidate，command_supported=false，不生成可提交命令。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R p7_agent_runtime_threat_no_submit --output-on-failure
rg -n "FleeResolver|GroupCombat|WarResolver|tribe_split|pack_combat" backend/include/pathfinder backend/src && exit 1 || true
```

## 19. TASK-P7-009：边界审计和开发笔记

任务名称：更新 Agent 开发笔记并执行 P7 边界审计。

允许修改：

```text
backend/dev_notes/agent.md
doc/review/P7Agent运行时最小闭环审核报告.md
```

审计要求：

```text
确认 Policy 不读取 GameState。
确认 Policy 不调用 RulePipeline。
确认 Builder 不调用 RulePipeline。
确认 Runtime 不直接修改 GameState。
确认 Runtime 只通过 AgentCommandAdapter 生成 CommandEnvelope。
确认 Runtime 只按 allowlist 提交 pipeline。
确认未新增 FleeResolver / GroupCombat / WarResolver。
确认未新增 H5 / HTTP / WebSocket / SaveManager / RL。
确认 P5/P6 回归测试通过。
确认 P7 集成测试通过。
```

验收命令：

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent|p7|p6|p5|p3|p4" --output-on-failure
ctest --test-dir build/backend --output-on-failure
rg -n "GameState|object_store|actor_store|RulePipeline|EatObjectResolver|edible_profile|hunger_delta|health_delta|effect_kind" backend/include/pathfinder/agent/policy.h backend/src/agent/policy.cpp backend/tests/unit/agent/agent_policy_test.cpp && exit 1 || true
rg -n "edible_profile|hunger_delta|health_delta|effect_kind" backend/include/pathfinder/agent backend/src/agent backend/tests/unit/agent backend/tests/integration/p7 && exit 1 || true
rg -n "FleeResolver|GroupCombat|WarResolver|tribe_split|pack_combat|rl_environment|BehaviorTree|GOAP|UtilityPolicy|NeuralPolicy|LLMPolicy|WebSocket|HTTP|SaveManager" backend/include/pathfinder/agent backend/src/agent backend/tests/integration/p7 && exit 1 || true
find backend -type d \( -path '*/frontend*' -o -path '*/http*' -o -path '*/websocket*' -o -path '*/save*' \) -print
```

审核报告必须包含：

```text
构建结果。
定向测试结果。
全量测试结果。
边界扫描结果。
是否允许进入 P8。
```

## 20. P7 设计风险和处理

### 20.1 风险：Runtime 变成完整 AI

处理：

```text
P7 只允许 FirstSupportedPolicy。
不做目标、效用、路径、战术、学习。
```

### 20.2 风险：Flee 被误提交成成功

处理：

```text
Runtime 必须使用 submit_action_allowlist。
P7 只 allow eat。
Flee/CallGroup 只做候选或命令适配验证，不做规则结算。
```

### 20.3 风险：Policy 偷看世界真相

处理：

```text
PolicyRequest 不包含 GameState。
边界扫描禁止 policy 文件出现 GameState/object_store/actor_store/edible_profile 等词。
```

### 20.4 风险：候选动作没有目标，Runtime 无法生成 intent

处理：

```text
P7 第一个实现任务就是补 candidate.suggested_targets 和 command_action_id。
严禁从 reason_key 解析目标。
```

### 20.5 风险：P7 破坏 P6 边界扫描

处理：

```text
P6 的 Builder 边界扫描仍只针对 Builder 文件。
P7 可以新增 runtime/policy 文件，但必须有 P7 自己的边界扫描。
```

## 21. P7 完成后的系统能力

P7 完成后，系统具备：

```text
一个 Agent 可以通过 Runtime 自动完成一次 tick。
观察和动作空间不再需要测试手写。
最小策略可以稳定选择候选动作。
CommandEnvelope 仍由 AgentCommandAdapter 统一生成。
RulePipeline 仍是唯一规则结算入口。
Runtime trace 能服务复盘、调试、后续训练环境。
Flee/CallGroup 等未实现规则不会被误判为成功。
```

P7 完成后仍不具备：

```text
复杂 AI。
多 Agent 批量调度。
长期 AgentState。
目标系统。
记忆检索策略。
传播/族群/文明。
H5 可视化。
强化学习环境。
```

## 22. P7 之后建议

P7 之后建议进入 P8：

```text
P8：Agent 运行记录与复盘兼容。
```

P8 目标建议：

```text
记录 AgentTickRecord / AgentDecisionRecord。
让 ReplayRunner 能复用历史 CommandEnvelope，不重新思考。
为后续训练环境准备 observation/action/decision/result 样本。
```

H5 建议放在 AgentRuntime 和 Replay 记录稳定之后再做，否则前端很容易只能显示手写 demo，而不是权威规则流。
