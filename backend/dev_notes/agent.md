# Agent 模块开发笔记 (P5 + P6 + P7)

## 模块职责

Agent 模块负责定义智能体的数据契约、命令适配器、以及观察/动作空间构建器。
P5 做最小闭环，P6 引入 ObservationBuilder 和 ActionSpaceBuilder。

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
